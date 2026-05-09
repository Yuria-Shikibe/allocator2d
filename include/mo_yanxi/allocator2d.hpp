#include <ranges>
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
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <map>
#include <set>
#include <unordered_map>
#include <memory>
#include <limits>
#include <scoped_allocator>
#include <optional>
#include <iostream>
#include <cassert>
#include <type_traits>
#include <vector>
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

#if defined(__has_cpp_attribute)
#if __has_cpp_attribute(msvc::forceinline)
#define MO_YANXI_ALLOCATOR_2D_FORCE_INLINE [[msvc::forceinline]]
#elif __has_cpp_attribute(gnu::always_inline)
#define MO_YANXI_ALLOCATOR_2D_FORCE_INLINE [[gnu::always_inline]]
#else
#define MO_YANXI_ALLOCATOR_2D_FORCE_INLINE
#endif
#else
#define MO_YANXI_ALLOCATOR_2D_FORCE_INLINE
#endif

#if defined(__has_cpp_attribute)
#if __has_cpp_attribute(no_unique_address)
#define MO_YANXI_ALLOCATOR_2D_NO_UNIQUE_ADDRESS [[no_unique_address]]
#else
#define MO_YANXI_ALLOCATOR_2D_NO_UNIQUE_ADDRESS
#endif
#else
#define MO_YANXI_ALLOCATOR_2D_NO_UNIQUE_ADDRESS
#endif


namespace mo_yanxi::math{
MO_YANXI_ALLOCATOR_2D_EXPORT
template <typename T>
struct vector2{
	T x;
	T y;

	MO_YANXI_ALLOCATOR_2D_FORCE_INLINE constexpr vector2 operator+(const vector2& other) const noexcept{
		return {x + other.x, y + other.y};
	}

	MO_YANXI_ALLOCATOR_2D_FORCE_INLINE constexpr vector2 operator-(const vector2& other) const noexcept{
		return {x - other.x, y - other.y};
	}

	constexpr bool operator==(const vector2& other) const noexcept = default;

	template <typename U>
	MO_YANXI_ALLOCATOR_2D_FORCE_INLINE constexpr vector2<U> as() const noexcept{
		return vector2<U>{static_cast<U>(x), static_cast<U>(y)};
	}

	MO_YANXI_ALLOCATOR_2D_FORCE_INLINE constexpr T area() const noexcept{
		return x * y;
	}

	MO_YANXI_ALLOCATOR_2D_FORCE_INLINE constexpr bool beyond(const vector2& other) const noexcept{
		return x > other.x || y > other.y;
	}
};

using usize2 = vector2<std::uint32_t>;
}

template <typename T>
struct std::hash<mo_yanxi::math::vector2<T>>{
	MO_YANXI_ALLOCATOR_2D_FORCE_INLINE MO_YANXI_ALLOCATOR_2D_CALL_STATIC std::size_t operator()(
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

	MO_YANXI_ALLOCATOR_2D_FORCE_INLINE [[nodiscard]] exchange_on_move() = default;

	MO_YANXI_ALLOCATOR_2D_FORCE_INLINE [[nodiscard]] exchange_on_move(const T& value)
		: value(value){
	}

	MO_YANXI_ALLOCATOR_2D_FORCE_INLINE exchange_on_move(const exchange_on_move& other) noexcept = default;

	MO_YANXI_ALLOCATOR_2D_FORCE_INLINE exchange_on_move(exchange_on_move&& other) noexcept
		: value{std::exchange(other.value, {})}{
	}

	MO_YANXI_ALLOCATOR_2D_FORCE_INLINE exchange_on_move& operator=(const exchange_on_move& other) noexcept = default;

	MO_YANXI_ALLOCATOR_2D_FORCE_INLINE exchange_on_move& operator=(const T& other) noexcept{
		value = other;
		return *this;
	}

	MO_YANXI_ALLOCATOR_2D_FORCE_INLINE exchange_on_move& operator=(exchange_on_move&& other) noexcept{
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
	using body_slot_type = size_type;
	static constexpr body_slot_type invalid_body_slot = std::numeric_limits<body_slot_type>::max();

	struct body_pool;
	using body_pool_allocator_type = typename std::allocator_traits<allocator_type>::template rebind_alloc<body_pool>;
	using body_pool_allocator_traits = std::allocator_traits<body_pool_allocator_type>;

	struct body_pool_deleter{
		void operator()(body_pool* pool) const noexcept;
	};

	using body_pool_owner_type = std::unique_ptr<body_pool, body_pool_deleter>;

	MO_YANXI_ALLOCATOR_2D_NO_UNIQUE_ADDRESS allocator_type allocator_{};
	exchange_on_move<extent_type> extent_{};
	exchange_on_move<large_size_type> remain_area_{};
	exchange_on_move<large_size_type> fragment_threshold_{};

	struct split_point;

	body_pool& ensure_body_pool_();
	[[nodiscard]] allocator2d& body_allocator_at_(body_slot_type slot);
	[[nodiscard]] const allocator2d& body_allocator_at_(body_slot_type slot) const;
	[[nodiscard]] body_slot_type acquire_body_slot_();
	void release_body_slot_(body_slot_type slot) noexcept;
	[[nodiscard]] allocator2d& create_body_allocator_(split_point& node);
	void destroy_body_allocator_(split_point& node) noexcept;

	struct allocation_record{
		point_type owner_point{};
		point_type nested_point{};
		extent_type extent{};
		bool nested{};
	};

	using map_type = std::unordered_map<
		point_type, split_point,
		std::hash<point_type>, std::equal_to<point_type>,
		typename std::allocator_traits<allocator_type>::template rebind_alloc<std::pair<
			const point_type, split_point>>
	>;

	using allocation_map_type = std::unordered_map<
		point_type, allocation_record,
		std::hash<point_type>, std::equal_to<point_type>,
		typename std::allocator_traits<allocator_type>::template rebind_alloc<std::pair<
			const point_type, allocation_record>>
	>;

	struct free_entry{
		size_type major{};
		size_type minor{};
		point_type point{};
	};

	struct free_entry_compare{
		bool operator()(const free_entry& lhs, const free_entry& rhs) const noexcept{
			if(lhs.major != rhs.major) return lhs.major < rhs.major;
			if(lhs.minor != rhs.minor) return lhs.minor < rhs.minor;
			if(lhs.point.y != rhs.point.y) return lhs.point.y < rhs.point.y;
			return lhs.point.x < rhs.point.x;
		}
	};

	using free_tree_type = std::multiset<
		free_entry,
		free_entry_compare,
		typename std::allocator_traits<allocator_type>::template rebind_alloc<free_entry>
	>;

	using index_handle = typename free_tree_type::iterator;

	struct region_index{
		free_tree_type xy{};
		free_tree_type yx{};

		region_index() = default;

		explicit region_index(const allocator_type& allocator)
			: xy(typename free_tree_type::allocator_type{allocator}),
			  yx(typename free_tree_type::allocator_type{allocator}){
		}
	};

	using body_nodes_type = std::vector<
		point_type,
		typename std::allocator_traits<allocator_type>::template rebind_alloc<point_type>>;

	struct split_point{
		point_type parent{};
		point_type bot_lft{};
		point_type top_rit{};
		point_type split{top_rit};

		bool idle{true};
		bool idle_top{true};
		bool idle_right{true};
		bool in_free_tree{false};
		bool in_fragment_tree{false};
		bool wide_top_split{false};
		bool is_top_child{false};

		body_slot_type body_slot{invalid_body_slot};

		index_handle free_xy{};
		index_handle free_yx{};

		[[nodiscard]] split_point() = default;

		[[nodiscard]] split_point(
			const point_type parent,
			const point_type bot_lft,
			const point_type top_rit)
			: parent(parent), bot_lft(bot_lft), top_rit(top_rit){
		}

		MO_YANXI_ALLOCATOR_2D_FORCE_INLINE [[nodiscard]] bool is_leaf() const noexcept{
			return split == top_rit;
		}

		MO_YANXI_ALLOCATOR_2D_FORCE_INLINE [[nodiscard]] bool is_split_idle() const noexcept{
			return idle_top && idle_right;
		}

		MO_YANXI_ALLOCATOR_2D_FORCE_INLINE [[nodiscard]] extent_type body_extent() const noexcept{
			return split - bot_lft;
		}

		MO_YANXI_ALLOCATOR_2D_FORCE_INLINE void clear_free_tree_state() noexcept{
			in_free_tree = false;
		}

		MO_YANXI_ALLOCATOR_2D_FORCE_INLINE [[nodiscard]] bool prefer_wide_top_split(const extent_type extent) const noexcept{
			const auto remain_x = top_rit.x - (bot_lft.x + extent.x);
			const auto remain_y = top_rit.y - (bot_lft.y + extent.y);
			return remain_x < remain_y;
		}

		MO_YANXI_ALLOCATOR_2D_FORCE_INLINE [[nodiscard]] point_type top_region_src() const noexcept{
			return {bot_lft.x, split.y};
		}

		MO_YANXI_ALLOCATOR_2D_FORCE_INLINE [[nodiscard]] point_type top_region_end() const noexcept{
			if(wide_top_split) return top_rit;
			return {split.x, top_rit.y};
		}

		MO_YANXI_ALLOCATOR_2D_FORCE_INLINE [[nodiscard]] point_type right_region_src() const noexcept{
			return {split.x, bot_lft.y};
		}

		MO_YANXI_ALLOCATOR_2D_FORCE_INLINE [[nodiscard]] point_type right_region_end() const noexcept{
			if(wide_top_split) return {top_rit.x, split.y};
			return top_rit;
		}

		MO_YANXI_ALLOCATOR_2D_FORCE_INLINE void mark_captured(allocator2d& alloc) noexcept{
			idle = false;

			split_point* cur = this;
			while(auto* parent = alloc.parent_node_of_(*cur)){
				if(cur->is_top_child){
					parent->idle_top = false;
				} else{
					parent->idle_right = false;
				}
				cur = parent;
			}
		}

		MO_YANXI_ALLOCATOR_2D_FORCE_INLINE allocator2d& ensure_body_allocator(allocator2d& alloc){
			assert(!is_leaf());
			if(body_slot == invalid_body_slot){
				alloc.erase_mark_(bot_lft, split);
				return alloc.create_body_allocator_(*this);
			}
			return alloc.body_allocator_at_(body_slot);
		}

		MO_YANXI_ALLOCATOR_2D_FORCE_INLINE void release_body_allocator(allocator2d& alloc) noexcept{
			if(body_slot == invalid_body_slot) return;
			alloc.destroy_body_allocator_(*this);
		}

		bool check_merge(allocator2d& alloc) noexcept{
			if(idle && is_split_idle()){
				if(body_slot != invalid_body_slot){
					assert(alloc.body_allocator_at_(body_slot).remain_area() == body_extent().template as<large_size_type>().area());
					this->release_body_allocator(alloc);
				}

				const point_type top_src = top_region_src();
				const point_type top_end = top_region_end();
				if((top_end - top_src).area() > 0) alloc.erase_split_(top_src, top_end);

				const point_type right_src = right_region_src();
				const point_type right_end = right_region_end();
				if((right_end - right_src).area() > 0) alloc.erase_split_(right_src, right_end);

				alloc.erase_mark_(bot_lft, split);
				split = top_rit;
				idle_top = true;
				idle_right = true;

				if(auto* parent = alloc.parent_node_of_(*this); parent != nullptr){
					if(is_top_child){
						parent->idle_top = true;
					} else{
						parent->idle_right = true;
					}
					return true;
				}
				return false;
			}
			return false;
		}

		void acquire_and_split(allocator2d& alloc, const extent_type extent){
			assert(idle);
			assert(is_leaf());

			split = bot_lft + extent;
			wide_top_split = prefer_wide_top_split(extent);

			alloc.erase_mark_(bot_lft, top_rit);

			const point_type right_src = right_region_src();
			const point_type right_end = right_region_end();
			if((right_end - right_src).area() > 0){
				alloc.add_split_(bot_lft, right_src, right_end);
			}

			const point_type top_src = top_region_src();
			const point_type top_end = top_region_end();
			if((top_end - top_src).area() > 0){
				alloc.add_split_(bot_lft, top_src, top_end);
			}

			mark_captured(alloc);
			clear_free_tree_state();
		}

		split_point* mark_idle(allocator2d& alloc) noexcept{
			assert(!idle);
			idle = true;
			split_point* p = this;
			split_point* last = this;
			while(p->check_merge(alloc)){
				auto* next = alloc.parent_node_of_(*p);
				assert(next != nullptr);
				last = p;
				p = next;
			}

			if(p->is_leaf()){
				alloc.mark_size_(p->bot_lft, p->split);
			} else{
				alloc.mark_size_(last->bot_lft, last->split);
			}
			return p;
		}

		void mark_body_idle(allocator2d& alloc) noexcept{
			idle = true;
			split_point* p = this;
			split_point* last = this;
			while(p->check_merge(alloc)){
				auto* next = alloc.parent_node_of_(*p);
				assert(next != nullptr);
				last = p;
				p = next;
			}

			if(p == this && !p->is_leaf()) return;
			if(p->is_leaf()){
				alloc.mark_size_(p->bot_lft, p->split);
			} else{
				alloc.mark_size_(last->bot_lft, last->split);
			}
		}
	};

	struct node_choice{
		std::optional<point_type> point{};
		extent_type extent{};
		large_size_type area{std::numeric_limits<large_size_type>::max()};
		size_type max_slack{std::numeric_limits<size_type>::max()};
		size_type min_slack{std::numeric_limits<size_type>::max()};
		split_point* nested_owner{};
		point_type nested_point{};
	};

	map_type map_{};
	allocation_map_type allocations_{};
	region_index large_nodes_{};
	region_index frag_nodes_{};
	body_nodes_type body_nodes_{};
	body_pool_owner_type body_pool_owner_{};

	MO_YANXI_ALLOCATOR_2D_FORCE_INLINE static bool better_choice_(const node_choice& lhs, const node_choice& rhs) noexcept{
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
	node_choice find_best_node_in_tree_(free_tree_type& tree, const extent_type size){
		node_choice best{};
		const auto outer_need = outer_is_x ? size.x : size.y;
		const auto inner_need = outer_is_x ? size.y : size.x;
		const auto max_size = std::numeric_limits<size_type>::max();

		auto make_probe = [](const size_type major, const size_type minor, const point_type point) noexcept{
			return free_entry{major, minor, point};
		};

		for(auto outer = tree.lower_bound(make_probe(outer_need, inner_need, {0, 0})); outer != tree.end();){
			const auto outer_size = outer->major;
			const auto optimistic_area = static_cast<large_size_type>(outer_size) * inner_need;
			if(best.point && optimistic_area > best.area) break;

			if(outer->minor < inner_need){
				outer = tree.lower_bound(make_probe(outer_size, inner_need, {0, 0}));
				continue;
			}

			const extent_type candidate_extent = outer_is_x
				? extent_type{outer_size, outer->minor}
				: extent_type{outer->minor, outer_size};
			const extent_type slack = candidate_extent - size;

			node_choice candidate{
				.point = outer->point,
				.extent = candidate_extent,
				.area = candidate_extent.as<large_size_type>().area(),
				.max_slack = std::max(slack.x, slack.y),
				.min_slack = std::min(slack.x, slack.y),
			};

			if(better_choice_(candidate, best)){
				best = candidate;
			}

			outer = tree.upper_bound(make_probe(outer_size, max_size, {max_size, max_size}));
		}

		return best;
	}

	MO_YANXI_ALLOCATOR_2D_FORCE_INLINE [[nodiscard]] bool is_fragment_(const point_type& size) const noexcept{
		return size.as<large_size_type>().area() <= fragment_threshold_.value;
	}

	node_choice find_best_node_(region_index& tree, const extent_type size){
		if(size.x >= size.y){
			return find_best_node_in_tree_<true>(tree.xy, size);
		}
		return find_best_node_in_tree_<false>(tree.yx, size);
	}

	node_choice find_best_direct_node_(const extent_type size){
		auto frag_node = find_best_node_(frag_nodes_, size);
		if(frag_node.point) return frag_node;
		return find_best_node_(large_nodes_, size);
	}

	node_choice find_best_candidate_(const extent_type size){
		const auto request_area = size.as<large_size_type>().area();
		node_choice best = find_best_direct_node_(size);
		if(best.point && best.area == request_area) return best;
		for(const auto& body_point : body_nodes_){
			auto* body_node = node_at_(body_point);
			if(body_node == nullptr || body_node->body_slot == invalid_body_slot) continue;
			auto& child = body_allocator_at_(body_node->body_slot);
			if(child.remain_area_.value < request_area) continue;

			auto nested = child.find_best_candidate_(size);
			if(!nested.point) continue;

			const point_type child_point = nested.point.value();
			node_choice candidate = nested;
			candidate.point = body_node->bot_lft + child_point;
			candidate.nested_owner = body_node;
			candidate.nested_point = child_point;

			if(better_choice_(candidate, best)){
				best = candidate;
				if(best.area == request_area) return best;
			}
		}
		return best;
	}

	MO_YANXI_ALLOCATOR_2D_FORCE_INLINE [[nodiscard]] split_point* node_at_(const point_type point) noexcept{
		auto itr = map_.find(point);
		if(itr == map_.end()) return nullptr;
		return &itr->second;
	}

	MO_YANXI_ALLOCATOR_2D_FORCE_INLINE [[nodiscard]] const split_point* node_at_(const point_type point) const noexcept{
		auto itr = map_.find(point);
		if(itr == map_.end()) return nullptr;
		return &itr->second;
	}

	MO_YANXI_ALLOCATOR_2D_FORCE_INLINE [[nodiscard]] split_point* parent_node_of_(split_point& node) noexcept{
		if(node.parent == node.bot_lft) return nullptr;
		return node_at_(node.parent);
	}

	MO_YANXI_ALLOCATOR_2D_FORCE_INLINE [[nodiscard]] const split_point* parent_node_of_(const split_point& node) const noexcept{
		if(node.parent == node.bot_lft) return nullptr;
		return node_at_(node.parent);
	}

	void mark_size_(const point_type src, const point_type dst){
		const auto size = dst - src;
		auto node_itr = map_.find(src);
		assert(node_itr != map_.end());
		auto& node = node_itr->second;

		if(is_fragment_(size)){
			node.free_xy = frag_nodes_.xy.insert({size.x, size.y, src});
			node.free_yx = frag_nodes_.yx.insert({size.y, size.x, src});
			node.in_fragment_tree = true;
		} else{
			node.free_xy = large_nodes_.xy.insert({size.x, size.y, src});
			node.free_yx = large_nodes_.yx.insert({size.y, size.x, src});
			node.in_fragment_tree = false;
		}

		node.in_free_tree = true;
	}

	void add_split_(const point_type parent, const point_type src, const point_type dst){
		auto [node_itr, inserted] = map_.try_emplace(src, parent, src, dst);
		assert(inserted);
		auto& node = node_itr->second;

		node.is_top_child = parent != src && src.x == parent.x;

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

		region_index& index = node.in_fragment_tree ? frag_nodes_ : large_nodes_;

		index.xy.erase(node.free_xy);
		index.yx.erase(node.free_yx);

		node.clear_free_tree_state();
	}

	void init_threshold_(const extent_type extent){
		if(!fragment_threshold_.value){
			fragment_threshold_ = std::max<large_size_type>(extent.as<large_size_type>().area() / 64, 96 * 96);
		}
	}

	MO_YANXI_ALLOCATOR_2D_FORCE_INLINE [[nodiscard]] large_size_type total_area_() const noexcept{
		return extent_.value.template as<large_size_type>().area();
	}

	MO_YANXI_ALLOCATOR_2D_FORCE_INLINE void register_body_node_(split_point* node){
		const auto point = node->bot_lft;
		if(std::ranges::find(body_nodes_, point) != body_nodes_.end()) return;
		body_nodes_.push_back(point);
	}

	MO_YANXI_ALLOCATOR_2D_FORCE_INLINE void unregister_body_node_(split_point* node) noexcept{
		std::erase(body_nodes_, node->bot_lft);
	}

	std::optional<point_type> allocate_local_(const extent_type extent){
		if(extent.area() == 0){
			return std::nullopt;
		}
		if(extent.beyond(extent_.value)) return std::nullopt;

		const auto area = extent.as<large_size_type>().area();
		if(remain_area_.value < area) return std::nullopt;

		const auto candidate = find_best_candidate_(extent);
		if(!candidate.point) return std::nullopt;

		if(candidate.nested_owner != nullptr){
			auto& nested_alloc = candidate.nested_owner->body_slot != invalid_body_slot
				? body_allocator_at_(candidate.nested_owner->body_slot)
				: candidate.nested_owner->ensure_body_allocator(*this);
			auto nested_point = nested_alloc.allocate_local_(extent);
			assert(nested_point.has_value());
			candidate.nested_owner->idle = false;
			auto [itr, inserted] = allocations_.try_emplace(
				candidate.point.value(),
				allocation_record{candidate.nested_owner->bot_lft, nested_point.value(), extent, true});
			assert(inserted);
			(void)itr;
		} else{
			auto node_itr = map_.find(candidate.point.value());
			assert(node_itr != map_.end());
			auto& node = node_itr->second;
			if(node.is_leaf()){
				node.acquire_and_split(*this, extent);
				auto [itr, inserted] = allocations_.try_emplace(
					candidate.point.value(), allocation_record{node.bot_lft, {}, extent, false});
				assert(inserted);
				(void)itr;
			} else{
				auto& nested_alloc = node.ensure_body_allocator(*this);
				auto nested_point = nested_alloc.allocate_local_(extent);
				assert(nested_point.has_value());
				node.idle = false;
				auto [itr, inserted] = allocations_.try_emplace(
					candidate.point.value(),
					allocation_record{node.bot_lft, nested_point.value(), extent, true});
				assert(inserted);
				(void)itr;
			}
		}

		remain_area_.value -= area;
		return candidate.point;
	}

	bool deallocate_local_(const point_type value) noexcept{
		const auto itr = allocations_.find(value);
		if(itr == allocations_.end()) return false;

		const allocation_record record = itr->second;
		allocations_.erase(itr);
		remain_area_.value += record.extent.area();

		if(record.nested){
			auto* owner = node_at_(record.owner_point);
			assert(owner != nullptr);
			assert(owner->body_slot != invalid_body_slot);
			auto& child = body_allocator_at_(owner->body_slot);
			const bool success = child.deallocate_local_(record.nested_point);
			assert(success);
			(void)success;
			if(child.remain_area() == child.extent().area()){
				owner->mark_body_idle(*this);
			}
		} else{
			auto* owner = node_at_(record.owner_point);
			assert(owner != nullptr);
			owner->mark_idle(*this);
		}
		return true;
	}

public:
	[[nodiscard]] allocator2d() = default;

	[[nodiscard]] explicit allocator2d(const allocator_type& allocator, large_size_type frag_thres = 0)
		: allocator_(allocator), fragment_threshold_(frag_thres),
		  map_(allocator), allocations_(allocator),
		  large_nodes_(allocator), frag_nodes_(allocator), body_nodes_(allocator){
	}

	[[nodiscard]] explicit allocator2d(const extent_type extent, large_size_type frag_thres = 0)
		: extent_(extent), remain_area_(extent.area()), fragment_threshold_(frag_thres){
		init_threshold_(extent);
		add_split_({}, {}, extent);
	}

	[[nodiscard]] allocator2d(const allocator_type& allocator, const extent_type extent, large_size_type frag_thres = 0)
		: allocator_(allocator), extent_(extent), remain_area_(extent.area()), fragment_threshold_(frag_thres),
		  map_(allocator), allocations_(allocator),
		  large_nodes_(allocator), frag_nodes_(allocator), body_nodes_(allocator){
		init_threshold_(extent);
		add_split_({}, {}, extent);
	}

	[[nodiscard]] std::optional<point_type> allocate(const extent_type extent){
		return allocate_local_(extent);
	}

	bool deallocate(const point_type value) noexcept{
		return deallocate_local_(value);
	}

	MO_YANXI_ALLOCATOR_2D_FORCE_INLINE [[nodiscard]] extent_type extent() const noexcept{ return extent_.value; }
	MO_YANXI_ALLOCATOR_2D_FORCE_INLINE [[nodiscard]] large_size_type remain_area() const noexcept{ return remain_area_.value; }

	allocator2d(allocator2d&& other) = default;

	allocator2d& operator=(allocator2d&& other) = default;

protected:
	allocator2d& operator=(const allocator2d& other) = default;
	allocator2d(const allocator2d& other) = default;

	[[nodiscard]] bool is_leak_() const noexcept{
		return this->remain_area() != total_area_();
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

template <typename Alloc>
struct allocator2d<Alloc>::body_pool{
	using allocator_pointer_vector_type = std::vector<
		allocator2d*,
		typename std::allocator_traits<allocator_type>::template rebind_alloc<allocator2d*>>;
	using free_slots_type = std::vector<
		body_slot_type,
		typename std::allocator_traits<allocator_type>::template rebind_alloc<body_slot_type>>;
	using body_allocator_type = typename std::allocator_traits<allocator_type>::template rebind_alloc<allocator2d>;
	using body_allocator_traits = std::allocator_traits<body_allocator_type>;

	MO_YANXI_ALLOCATOR_2D_NO_UNIQUE_ADDRESS allocator_type allocator;
	allocator_pointer_vector_type allocators;
	free_slots_type free_slots;

	explicit body_pool(const allocator_type& allocator)
		: allocator(allocator), allocators(allocator), free_slots(allocator){
	}

	body_pool(const body_pool&) = delete;
	body_pool& operator=(const body_pool&) = delete;

	~body_pool(){
		for(auto*& allocator : allocators){
			if(allocator == nullptr) continue;
			destroy_allocator(allocator);
			allocator = nullptr;
		}
	}

	MO_YANXI_ALLOCATOR_2D_FORCE_INLINE [[nodiscard]] allocator2d* create_allocator(const extent_type extent){
		body_allocator_type body_allocator(allocator);
		auto* child = body_allocator_traits::allocate(body_allocator, 1);
		try{
			body_allocator_traits::construct(body_allocator, child, allocator, extent);
		} catch(...){
			body_allocator_traits::deallocate(body_allocator, child, 1);
			throw;
		}
		return child;
	}

	MO_YANXI_ALLOCATOR_2D_FORCE_INLINE void destroy_allocator(allocator2d* allocator_ptr) noexcept{
		body_allocator_type body_allocator(allocator);
		body_allocator_traits::destroy(body_allocator, allocator_ptr);
		body_allocator_traits::deallocate(body_allocator, allocator_ptr, 1);
	}
};

template <typename Alloc>
void allocator2d<Alloc>::body_pool_deleter::operator()(body_pool* pool) const noexcept{
	if(pool == nullptr) return;
	body_pool_allocator_type allocator(pool->allocator);
	body_pool_allocator_traits::destroy(allocator, pool);
	body_pool_allocator_traits::deallocate(allocator, pool, 1);
}

template <typename Alloc>
typename allocator2d<Alloc>::body_pool& allocator2d<Alloc>::ensure_body_pool_(){
	if(!body_pool_owner_){
		body_pool_allocator_type pool_allocator(allocator_);
		auto* pool = body_pool_allocator_traits::allocate(pool_allocator, 1);
		try{
			body_pool_allocator_traits::construct(pool_allocator, pool, allocator_);
		} catch(...){
			body_pool_allocator_traits::deallocate(pool_allocator, pool, 1);
			throw;
		}
		body_pool_owner_.reset(pool);
	}
	return *body_pool_owner_;
}

template <typename Alloc>
allocator2d<Alloc>& allocator2d<Alloc>::body_allocator_at_(const body_slot_type slot){
	auto& pool = ensure_body_pool_();
	assert(slot < pool.allocators.size());
	auto& entry = pool.allocators[slot];
	assert(entry != nullptr);
	return *entry;
}

template <typename Alloc>
const allocator2d<Alloc>& allocator2d<Alloc>::body_allocator_at_(const body_slot_type slot) const{
	assert(body_pool_owner_ != nullptr);
	const auto& pool = *body_pool_owner_;
	assert(slot < pool.allocators.size());
	const auto& entry = pool.allocators[slot];
	assert(entry != nullptr);
	return *entry;
}

template <typename Alloc>
typename allocator2d<Alloc>::body_slot_type allocator2d<Alloc>::acquire_body_slot_(){
	auto& pool = ensure_body_pool_();
	if(!pool.free_slots.empty()){
		const auto slot = pool.free_slots.back();
		pool.free_slots.pop_back();
		assert(slot < pool.allocators.size());
		assert(pool.allocators[slot] == nullptr);
		return slot;
	}

	const auto slot = static_cast<body_slot_type>(pool.allocators.size());
	assert(slot != invalid_body_slot);
	pool.allocators.push_back(nullptr);
	return slot;
}

template <typename Alloc>
void allocator2d<Alloc>::release_body_slot_(const body_slot_type slot) noexcept{
	assert(body_pool_owner_ != nullptr);
	auto& pool = *body_pool_owner_;
	assert(slot < pool.allocators.size());
	assert(pool.allocators[slot] == nullptr);
	pool.free_slots.push_back(slot);
}

template <typename Alloc>
allocator2d<Alloc>& allocator2d<Alloc>::create_body_allocator_(split_point& node){
	assert(node.body_slot == invalid_body_slot);
	const auto slot = acquire_body_slot_();
	auto& pool = *body_pool_owner_;
	auto& entry = pool.allocators[slot];
	assert(entry == nullptr);
	try{
		entry = pool.create_allocator(node.body_extent());
	} catch(...){
		release_body_slot_(slot);
		throw;
	}
	auto& child = *entry;
	node.body_slot = slot;
	register_body_node_(&node);
	return child;
}

template <typename Alloc>
void allocator2d<Alloc>::destroy_body_allocator_(split_point& node) noexcept{
	assert(node.body_slot != invalid_body_slot);
	assert(body_pool_owner_ != nullptr);
	const auto slot = node.body_slot;
	auto& pool = *body_pool_owner_;
	assert(slot < pool.allocators.size());
	auto& entry = pool.allocators[slot];
	assert(entry != nullptr);
	unregister_body_node_(&node);
	pool.destroy_allocator(entry);
	entry = nullptr;
	release_body_slot_(slot);
	node.body_slot = invalid_body_slot;
}

MO_YANXI_ALLOCATOR_2D_EXPORT
template <typename Alloc = std::allocator<std::byte>>
struct allocator2d_checked : allocator2d<Alloc>{
	[[nodiscard]] allocator2d_checked(const typename allocator2d<Alloc>::allocator_type& allocator,
	                                  typename allocator2d<Alloc>::large_size_type frag_thres = 0)
		: allocator2d<Alloc>(allocator, frag_thres){
	}

	[[nodiscard]] allocator2d_checked(const typename allocator2d<Alloc>::extent_type& extent,
	                                  typename allocator2d<Alloc>::large_size_type frag_thres = 0)
		: allocator2d<Alloc>(extent, frag_thres){
	}

	[[nodiscard]] allocator2d_checked(const typename allocator2d<Alloc>::allocator_type& allocator,
	                                  const typename allocator2d<Alloc>::extent_type& extent,
	                                  typename allocator2d<Alloc>::large_size_type frag_thres = 0)
		: allocator2d<Alloc>(allocator, extent, frag_thres){
	}

	[[nodiscard]] allocator2d_checked() = default;

	~allocator2d_checked(){
		this->check_leak_();
	}

	allocator2d_checked(allocator2d_checked&& other) noexcept(std::is_nothrow_move_constructible_v<allocator2d<Alloc>>) = default;

	allocator2d_checked& operator=(allocator2d_checked&& other) noexcept(std::is_nothrow_move_assignable_v<allocator2d<Alloc>>){
		if(this == &other) return *this;
		this->check_leak_();
		allocator2d<Alloc>::operator=(std::move(other));
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
#undef MO_YANXI_ALLOCATOR_2D_FORCE_INLINE
#undef MO_YANXI_ALLOCATOR_2D_NO_UNIQUE_ADDRESS
