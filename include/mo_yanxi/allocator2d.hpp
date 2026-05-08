#ifdef MO_YANXI_ALLOCATOR_2D_ENABLE_MODULE
#define MO_YANXI_ALLOCATOR_2D_EXPORT export
#else
#pragma once
#define MO_YANXI_ALLOCATOR_2D_EXPORT
#endif

#ifdef MO_YANXI_ALLOCATOR_2D_USE_STD_MODULE
import std;
#else
#include <utility>
#include <version>
#include <map>
#include <unordered_map>
#include <memory>
#include <limits>
#include <scoped_allocator>
#include <optional>
#include <iostream>
#include <cassert>
#endif


#ifdef MO_YANXI_ALLOCATOR_2D_HAS_MATH_VECTOR2
import mo_yanxi.math.vector2;
#else

#ifdef __cpp_static_call_operator
#define MO_YANXI_ALLOCATOR_2D_CALL_STATIC static
#define MO_YANXI_ALLOCATOR_2D_CALL_CONST
#else
#define MO_YANXI_ALLOCATOR_2D_CALL_STATIC
#define MO_YANXI_ALLOCATOR_2D_CALL_CONST const
#endif


namespace mo_yanxi::math{
MO_YANXI_ALLOCATOR_2D_EXPORT
template <typename T>
struct vector2{
	T x;
	T y;

	constexpr vector2 operator+(const vector2& other) const noexcept{
		return {x + other.x, y + other.y};
	}

	constexpr vector2 operator-(const vector2& other) const noexcept{
		return {x - other.x, y - other.y};
	}

	constexpr bool operator==(const vector2& other) const noexcept = default;

	template <typename U>
	constexpr vector2<U> as() const noexcept{
		return vector2<U>{static_cast<U>(x), static_cast<U>(y)};
	}

	constexpr T area() const noexcept{
		return x * y;
	}

	constexpr bool beyond(const vector2& other) const noexcept{
		return x > other.x || y > other.y;
	}
};

using usize2 = vector2<std::uint32_t>;
}

template <typename T>
struct std::hash<mo_yanxi::math::vector2<T>>{
	MO_YANXI_ALLOCATOR_2D_CALL_STATIC std::size_t operator()(
		const mo_yanxi::math::vector2<T>& v) MO_YANXI_ALLOCATOR_2D_CALL_CONST noexcept{
		std::size_t seed = 0;
		auto hash_combine = [&seed](std::size_t v){
			seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		};
		hash_combine(std::hash<T>{}(v.x));
		hash_combine(std::hash<T>{}(v.y));
		return seed;
	}
};

#endif


namespace mo_yanxi{
template <typename T>
struct exchange_on_move{
	T value;

	[[nodiscard]] exchange_on_move() = default;

	[[nodiscard]] exchange_on_move(const T& value)
		: value(value){
	}

	exchange_on_move(const exchange_on_move& other) noexcept = default;

	exchange_on_move(exchange_on_move&& other) noexcept
		: value{std::exchange(other.value, {})}{
	}

	exchange_on_move& operator=(const exchange_on_move& other) noexcept = default;

	exchange_on_move& operator=(const T& other) noexcept{
		value = other;
		return *this;
	}

	exchange_on_move& operator=(exchange_on_move&& other) noexcept{
		value = std::exchange(other.value, {});
		return *this;
	}
};
}

namespace mo_yanxi{
MO_YANXI_ALLOCATOR_2D_EXPORT
template <typename Alloc = std::allocator<std::byte>>
struct allocator2d{
private:
	using T = std::uint32_t;

public:
	using size_type = T;
	using large_size_type = std::uint64_t;
	using extent_type = math::vector2<T>;
	using point_type = math::vector2<T>;
	using allocator_type = Alloc;

private:
	exchange_on_move<extent_type> extent_{};
	exchange_on_move<large_size_type> remain_area_{};
	exchange_on_move<large_size_type> fragment_threshold_{};

	struct split_point;

	using map_type = std::unordered_map<
		point_type, split_point,
		std::hash<point_type>, std::equal_to<point_type>,
		typename std::allocator_traits<allocator_type>::template rebind_alloc<std::pair<
			const point_type, split_point>>
	>;
	map_type map_{};

	using inner_tree_type = std::multimap<
		size_type, point_type,
		std::less<size_type>,
		typename std::allocator_traits<allocator_type>::template rebind_alloc<std::pair<
			const size_type, point_type>>
	>;

	using tree_type = std::map<
		size_type, inner_tree_type,
		std::less<size_type>,
		std::scoped_allocator_adaptor<typename std::allocator_traits<allocator_type>::template rebind_alloc<
			std::pair<const size_type, inner_tree_type>>>
	>;

	tree_type large_nodes_xy_{};
	tree_type large_nodes_yx_{};

	tree_type frag_nodes_xy_{};
	tree_type frag_nodes_yx_{};

	using itr_outer = typename tree_type::iterator;
	using itr_inner = typename tree_type::mapped_type::iterator;

	struct index_handle{
		itr_outer outer{};
		itr_inner inner{};
	};

	struct itr_pair{
		itr_outer outer{};
		itr_inner inner{};

		[[nodiscard]] auto value() const{
			return inner->second;
		}

		void locate_next_inner(const size_type second){
			inner = outer->second.lower_bound(second);
		}

		bool locate_next_outer(tree_type& tree){
			++outer;
			if(outer == tree.end()) return false;
			return true;
		}

		[[nodiscard]] bool valid(const tree_type& tree) const noexcept{
			return outer != tree.end() && inner != outer->second.end();
		}
	};

	struct split_point{
		split_point* parent_node{};
		point_type parent{};
		point_type bot_lft{};
		point_type top_rit{};
		point_type split{top_rit};

		extent_type used_extent{};

		bool idle{true};
		bool idle_top{true};
		bool idle_right{true};
		bool in_free_tree{false};
		bool in_fragment_tree{false};
		bool wide_top_split{false};
		bool is_top_child{false};

		index_handle free_xy{};
		index_handle free_yx{};

		[[nodiscard]] split_point() = default;

		[[nodiscard]] split_point(
			const point_type parent,
			const point_type bot_lft,
			const point_type top_rit)
			: parent(parent), bot_lft(bot_lft), top_rit(top_rit){
		}

		[[nodiscard]] bool is_leaf() const noexcept{
			return split == top_rit;
		}

		[[nodiscard]] bool is_root() const noexcept{
			return parent_node == nullptr;
		}

		[[nodiscard]] bool is_split_idle() const noexcept{
			return idle_top && idle_right;
		}

		void clear_free_tree_state() noexcept{
			in_free_tree = false;
		}

		[[nodiscard]] bool prefer_wide_top_split(const extent_type extent) const noexcept{
			const auto remain_x = top_rit.x - (bot_lft.x + extent.x);
			const auto remain_y = top_rit.y - (bot_lft.y + extent.y);
			return remain_x < remain_y;
		}

		[[nodiscard]] point_type top_region_src() const noexcept{
			return {bot_lft.x, split.y};
		}

		[[nodiscard]] point_type top_region_end() const noexcept{
			if(wide_top_split) return top_rit;
			return {split.x, top_rit.y};
		}

		[[nodiscard]] point_type right_region_src() const noexcept{
			return {split.x, bot_lft.y};
		}

		[[nodiscard]] point_type right_region_end() const noexcept{
			if(wide_top_split) return {top_rit.x, split.y};
			return top_rit;
		}

		void mark_captured() noexcept{
			idle = false;

			split_point* cur = this;
			while(cur->parent_node != nullptr){
				split_point& p = *cur->parent_node;
				if(cur->is_top_child){
					p.idle_top = false;
				} else{
					p.idle_right = false;
				}
				cur = &p;
			}
		}

		bool check_merge(allocator2d& alloc) noexcept{
			if(idle && is_split_idle()){
				// 1. Top Region
				const point_type top_src = top_region_src();
				const point_type top_end = top_region_end();
				if((top_end - top_src).area() > 0) alloc.erase_split_(top_src, top_end);

				// 2. Right Region
				const point_type right_src = right_region_src();
				const point_type right_end = right_region_end();
				if((right_end - right_src).area() > 0) alloc.erase_split_(right_src, right_end);

				// Erase self from fragments/large pools if it was there
				// (For a leaf, it might be. For a parent attempting merge, it isn't.)
				alloc.erase_mark_(bot_lft, split);

				split = top_rit;

				if(parent_node == nullptr) return false;
				auto& p = get_parent();
				if(is_top_child){
					p.idle_top = true;
				} else{
					p.idle_right = true;
				}
				return true;
			} else{
				return false;
			}
		}

		split_point& get_parent() const noexcept{
			assert(parent_node != nullptr);
			return *parent_node;
		}

		void acquire_and_split(allocator2d& alloc, const math::usize2 extent){
			assert(idle);

			used_extent = extent;
			if(is_leaf()){
				// first split
				split = bot_lft + extent;
				wide_top_split = prefer_wide_top_split(extent);

				alloc.erase_mark_(bot_lft, top_rit);

				// 1. Right Region
				const point_type right_src = right_region_src();
				const point_type right_end = right_region_end();
				if((right_end - right_src).area() > 0){
					alloc.add_split_(bot_lft, right_src, right_end);
				}

				// 2. Top Region
				const point_type top_src = top_region_src();
				const point_type top_end = top_region_end();
				if((top_end - top_src).area() > 0){
					alloc.add_split_(bot_lft, top_src, top_end);
				}
			} else{
				alloc.erase_mark_(bot_lft, split);
			}

			this->mark_captured();
			this->clear_free_tree_state();
		}

		split_point* mark_idle(allocator2d& alloc) noexcept{
			assert(!idle);
			idle = true;
			used_extent = {};
			split_point* p = this;
			split_point* last = this;
			while(p->check_merge(alloc)){
				auto* next = p->parent_node;
				assert(next != nullptr);
				last = p;
				p = next;
			}

			if(const auto last_itr = alloc.map_.find(last->bot_lft); last_itr != alloc.map_.end()){
				assert(last_itr->second.idle);
			}
			if(p->is_leaf()){
				alloc.mark_size_(p->bot_lft, p->split);
			} else{
				alloc.mark_size_(last->bot_lft, last->split);
			}
			return p;
		}
	};

	struct node_choice{
		std::optional<point_type> point{};
		extent_type extent{};
		large_size_type area{std::numeric_limits<large_size_type>::max()};
		size_type max_slack{std::numeric_limits<size_type>::max()};
		size_type min_slack{std::numeric_limits<size_type>::max()};
	};

	static bool better_choice_(const node_choice& lhs, const node_choice& rhs) noexcept{
		if(!lhs.point) return false;
		if(!rhs.point) return true;
		if(lhs.area != rhs.area) return lhs.area < rhs.area;
		if(lhs.max_slack != rhs.max_slack) return lhs.max_slack < rhs.max_slack;
		if(lhs.min_slack != rhs.min_slack) return lhs.min_slack < rhs.min_slack;
		return lhs.point.value() == rhs.point.value()
			       ? false
			       : lhs.point.value().y < rhs.point.value().y ||
			       (lhs.point.value().y == rhs.point.value().y && lhs.point.value().x < rhs.point.value().x);
	}

	template <bool outer_is_x>
	node_choice find_best_node_in_tree_(tree_type& tree, const extent_type size){
		node_choice best{};
		const auto outer_need = outer_is_x ? size.x : size.y;
		const auto inner_need = outer_is_x ? size.y : size.x;

		for(auto outer = tree.lower_bound(outer_need); outer != tree.end(); ++outer){
			const auto outer_size = outer->first;
			const auto optimistic_area = static_cast<large_size_type>(outer_size) * inner_need;
			if(best.point && optimistic_area > best.area) break;

			auto inner = outer->second.lower_bound(inner_need);
			if(inner == outer->second.end()) continue;

			const extent_type candidate_extent = outer_is_x
				                                     ? extent_type{outer_size, inner->first}
				                                     : extent_type{inner->first, outer_size};
			const extent_type slack = candidate_extent - size;

			node_choice candidate{
					.point = inner->second,
					.extent = candidate_extent,
					.area = candidate_extent.as<large_size_type>().area(),
					.max_slack = std::max(slack.x, slack.y),
					.min_slack = std::min(slack.x, slack.y),
				};

			if(better_choice_(candidate, best)){
				best = candidate;
			}
		}

		return best;
	}

	[[nodiscard]] bool is_fragment_(const point_type& size) const noexcept{
		return size.as<large_size_type>().area() <= fragment_threshold_.value;
	}

	node_choice find_best_node_(tree_type& tree_xy, tree_type& tree_yx, const extent_type size){
		if(size.x >= size.y){
			return find_best_node_in_tree_<true>(tree_xy, size);
		}

		return find_best_node_in_tree_<false>(tree_yx, size);
	}

	std::optional<point_type> get_valid_node_(const extent_type size){
		assert(size.area() > 0);

		if(remain_area_.value < size.area()){
			return std::nullopt;
		}

		auto frag_node = find_best_node_(frag_nodes_xy_, frag_nodes_yx_, size);
		if(frag_node.point) return frag_node.point;

		return find_best_node_(large_nodes_xy_, large_nodes_yx_, size).point;
	}

	void mark_size_(const point_type src, const point_type dst){
		const auto size = dst - src;
		auto node_itr = map_.find(src);
		assert(node_itr != map_.end());
		auto& node = node_itr->second;

		if(is_fragment_(size)){
			auto [outer_xy, inserted_xy] = frag_nodes_xy_.try_emplace(size.x);
			auto [outer_yx, inserted_yx] = frag_nodes_yx_.try_emplace(size.y);
			(void)inserted_xy;
			(void)inserted_yx;
			node.free_xy = {outer_xy, outer_xy->second.insert({size.y, src})};
			node.free_yx = {outer_yx, outer_yx->second.insert({size.x, src})};
			node.in_fragment_tree = true;
		} else{
			auto [outer_xy, inserted_xy] = large_nodes_xy_.try_emplace(size.x);
			auto [outer_yx, inserted_yx] = large_nodes_yx_.try_emplace(size.y);
			(void)inserted_xy;
			(void)inserted_yx;
			node.free_xy = {outer_xy, outer_xy->second.insert({size.y, src})};
			node.free_yx = {outer_yx, outer_yx->second.insert({size.x, src})};
			node.in_fragment_tree = false;
		}

		node.in_free_tree = true;
	}

	void add_split_(const point_type parent, const point_type src, const point_type dst){
		auto [node_itr, inserted] = map_.try_emplace(src, parent, src, dst);
		assert(inserted);
		auto& node = node_itr->second;

		if(parent == src){
			node.parent_node = nullptr;
			node.is_top_child = false;
		} else{
			auto parent_itr = map_.find(parent);
			assert(parent_itr != map_.end());
			node.parent_node = &parent_itr->second;
			node.is_top_child = src.x == parent_itr->second.bot_lft.x;
		}

		mark_size_(src, dst);
	}

	void erase_split_(const point_type src, const point_type dst){
		erase_mark_(src, dst);
		map_.erase(src);
	}

	void erase_mark_(const point_type src, const point_type dst){
		(void)dst;
		auto node_itr = map_.find(src);
		if(node_itr == map_.end()) return;
		auto& node = node_itr->second;
		if(!node.in_free_tree) return;

		tree_type& tree_xy = node.in_fragment_tree ? frag_nodes_xy_ : large_nodes_xy_;
		tree_type& tree_yx = node.in_fragment_tree ? frag_nodes_yx_ : large_nodes_yx_;

		node.free_xy.outer->second.erase(node.free_xy.inner);
		if(node.free_xy.outer->second.empty()){
			tree_xy.erase(node.free_xy.outer);
		}

		node.free_yx.outer->second.erase(node.free_yx.inner);
		if(node.free_yx.outer->second.empty()){
			tree_yx.erase(node.free_yx.outer);
		}

		node.clear_free_tree_state();
	}

	static void erase_(tree_type& map, const point_type src, const size_type outer_key,
	                   const size_type inner_key) noexcept{
		auto itr = map.find(outer_key);
		if(itr == map.end()) return;

		auto& inner = itr->second;
		auto [begin, end] = inner.equal_range(inner_key);

		for(auto cur = begin; cur != end; ++cur){
			if(cur->second == src){
				inner.erase(cur);
				if(itr->second.empty()){
					map.erase(itr);
				}
				return;
			}
		}
	}

	void init_threshold_(const extent_type extent){
		if(!fragment_threshold_.value) fragment_threshold_ = std::max<large_size_type>(
			extent.as<large_size_type>().area() / 64, 96 * 96);
	}

public:
	[[nodiscard]] allocator2d() = default;

	[[nodiscard]] explicit allocator2d(const allocator_type& allocator, large_size_type frag_thres = 0)
		: fragment_threshold_(frag_thres),
		  map_(allocator), large_nodes_xy_(allocator),
		  large_nodes_yx_(allocator), frag_nodes_xy_(allocator), frag_nodes_yx_(allocator){
	}

	[[nodiscard]] explicit allocator2d(const extent_type extent, large_size_type frag_thres = 0)
		: extent_(extent), remain_area_(extent.area()), fragment_threshold_(frag_thres){
		init_threshold_(extent);
		add_split_({}, {}, extent);
	}

	[[nodiscard]] allocator2d(const allocator_type& allocator, const extent_type extent, large_size_type frag_thres = 0)
		: extent_(extent), remain_area_(extent.area()), fragment_threshold_(frag_thres), map_(allocator),
		  large_nodes_xy_(allocator), large_nodes_yx_(allocator),
		  frag_nodes_xy_(allocator), frag_nodes_yx_(allocator){
		init_threshold_(extent);
		add_split_({}, {}, extent);
	}

	[[nodiscard]] std::optional<point_type> allocate(const math::usize2 extent){
		if(extent.area() == 0){
			return std::nullopt;
		}

		if(extent.beyond(extent_.value)) return std::nullopt;
		if(remain_area_.value < extent.as<std::uint64_t>().area()) return std::nullopt;

		auto chamber_src = get_valid_node_(extent);
		if(!chamber_src) return std::nullopt;

		auto& chamber = map_.at(chamber_src.value());
		chamber.acquire_and_split(*this, extent);
		remain_area_.value -= extent.area();
		return chamber_src.value();
	}

	bool deallocate(const point_type value) noexcept{
		if(const auto itr = map_.find(value); itr != map_.end()){
			const auto extent = (itr->second.split - value).area();
			remain_area_.value += itr->second.used_extent.area();
			itr->second.mark_idle(*this);
			return true;
		}

		return false;
	}

	[[nodiscard]] extent_type extent() const noexcept{ return extent_.value; }
	[[nodiscard]] large_size_type remain_area() const noexcept{ return remain_area_.value; }

	allocator2d(allocator2d&& other) noexcept = default;
	allocator2d& operator=(allocator2d&& other) noexcept = default;

protected:
	allocator2d& operator=(const allocator2d& other) = default;
	allocator2d(const allocator2d& other) = default;

	[[nodiscard]] bool is_leak_() const noexcept{
		return this->remain_area() != this->extent().template as<large_size_type>().area();
	}

	void check_leak_() const noexcept{
		if(is_leak_()){
#ifdef MO_YANXI_ALLOCATOR_2D_LEAK_BEHAVIOR
			MO_YANXI_ALLOCATOR_2D_LEAK_BEHAVIOR(*this)
#else
			std::cerr << "Allocator2D Leaked!";
			std::terminate();
#endif
		}
	}
};

MO_YANXI_ALLOCATOR_2D_EXPORT
template <typename Alloc = std::allocator<std::byte>>
struct allocator2d_checked : allocator2d<Alloc>{
	[[nodiscard]] allocator2d_checked(const allocator2d<Alloc>::allocator_type& allocator,
	                                  allocator2d<Alloc>::large_size_type frag_thres = 0)
		: allocator2d<Alloc>(allocator, frag_thres){
	}

	[[nodiscard]] allocator2d_checked(const allocator2d<Alloc>::extent_type& extent,
	                                  allocator2d<Alloc>::large_size_type frag_thres = 0)
		: allocator2d<Alloc>(extent, frag_thres){
	}

	[[nodiscard]] allocator2d_checked(const allocator2d<Alloc>::allocator_type& allocator,
	                                  const allocator2d<Alloc>::extent_type& extent,
	                                  allocator2d<Alloc>::large_size_type frag_thres = 0)
		: allocator2d<Alloc>(allocator, extent, frag_thres){
	}

	[[nodiscard]] allocator2d_checked() = default;

	~allocator2d_checked(){
		this->check_leak_();
	}

	allocator2d_checked(allocator2d_checked&& other) noexcept(std::is_nothrow_move_constructible_v<allocator2d<Alloc>>)
	= default;

	allocator2d_checked& operator=(
		allocator2d_checked&& other) noexcept(std::is_nothrow_move_assignable_v<allocator2d<Alloc>>){
		if(this == &other) return *this;
		this->check_leak_();
		allocator2d<Alloc>::operator =(std::move(other));
		return *this;
	}

protected:
	allocator2d_checked& operator=(const allocator2d_checked& other) = default;
	allocator2d_checked(const allocator2d_checked& other) = default;
};
}
#undef MO_YANXI_ALLOCATOR_2D_EXPORT
#undef MO_YANXI_ALLOCATOR_2D_CALL_STATIC
#undef MO_YANXI_ALLOCATOR_2D_CALL_CONST
