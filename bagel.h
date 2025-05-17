// Copyright (C) 2025 Moshe Sulamy

#pragma once
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <type_traits>

namespace bagel
{
	struct Bagel
	{
		bool	AggregateUpdates = true;
		bool	CallbackOnDestroy = true;
		bool	DynamicResize = false;
		int		IdBagSize = 5;
		int		InitialEntities = 10;
		int		InitialPackedSize = 5;
		int		MaxComponents = 10;
	};

	template <class T> struct Storage;
	template <class T> class PackedStorage;
	template <class T> class SparseStorage;
	template <class T> class TaggedStorage;

#if __has_include("bagel_cfg.h")
	#define BAGEL_STORAGE(C,T) template <> struct Storage<C> { using type = T<C>; };
	#include "bagel_cfg.h"
	#undef BAGEL_STORAGE
#else
	constexpr Bagel Params{};
#endif

	using id_type = int;
	struct ent_type { id_type id; };
	using size_type = int;
	using index_type = int;
	using mask_type =
		std::conditional_t<Params.MaxComponents<=8, std::uint_fast8_t,
		std::conditional_t<Params.MaxComponents<=16, std::uint_fast16_t,
		std::conditional_t<Params.MaxComponents<=32, std::uint_fast32_t,
			std::uint_fast64_t>>>;
	constexpr inline size_type BitsetWidth = sizeof(mask_type)*8;

	class NoInstance { NoInstance() = delete; };
	struct NoCopy {
		NoCopy() = default;
		NoCopy(const NoCopy&) = delete;
		void operator=(const NoCopy&) = delete;
	};

	template <class T, int N>
	class DynamicBag : NoCopy
	{
	public:
		void push(const T& t) {
			if (_size == _capacity) {
				_capacity *= 2;
				_arr = static_cast<T*>(
					realloc(_arr, sizeof(T)*_capacity));
			}
			_arr[_size] = t;
			++_size;
		}
		void ensure(size_type s) {
			if (_capacity < s) {
				_capacity = std::max(s, _capacity*2);
				_arr = static_cast<T*>(
					realloc(_arr, sizeof(T)*_capacity));
			}
		}
		T pop() { return _arr[--_size]; }
		T& operator[](index_type i) { return _arr[i]; }
		const T& operator[](index_type i) const { return _arr[i]; }
		void clear() { _size = 0; }

		size_type size() const { return _size; }
		size_type capacity() const { return _capacity; }

		~DynamicBag() { free(_arr); }
	private:
		T*			_arr = static_cast<T*>(malloc(sizeof(T) * N));
		size_type	_size = 0;
		size_type	_capacity = N;
	};
	template <class T, int N>
	class StaticBag
	{
	public:
		void push(const T& t) { _arr[_size++] = t; }
		T pop() { return _arr[--_size]; }
		T& operator[](index_type i) { return _arr[i]; }
		const T& operator[](index_type i) const { return _arr[i]; }
		void clear() { _size = 0; }

		size_type size() const { return _size; }
		static constexpr size_type capacity() { return N; }
		static void ensure(size_type) {}
	private:
		T			_arr[N];
		size_type	_size = 0;
	};
	template <class T, int N>
	using Bag = std::conditional_t<Params.DynamicResize, DynamicBag<T, N>, StaticBag<T,N>>;

	struct StorageCallbacks
	{
		using Destroy = void (*)(ent_type);
		Destroy destroy = nullptr;
	};
	template <class> class StorageRegister;

	template <class T>
	class SparseStorage final : NoInstance
	{
	public:
		static void add(ent_type e, const T& t) {
			_bag.ensure(e.id);
			_bag[e.id] = t;
		}
		static void del(ent_type) {}
		static T& get(ent_type e) { return _bag[e.id]; }
	private:
		static inline Bag<T,Params.InitialEntities> _bag;
	};
	template <class T>
	class PackedStorage final : NoInstance
	{
	public:
		static void add(ent_type e, const T& t) {
			_entToComp.ensure(e.id);
			_entToComp[e.id] = _comps.size();
			_comps.push(t);
			_compToEnt.push(e);
		}
		static void del(ent_type e) {
			index_type ent_comp_idx = _entToComp[e.id];
			ent_type last_ent = _compToEnt.pop();

			_comps[ent_comp_idx] = _comps.pop();
			_compToEnt[ent_comp_idx] = last_ent;
			_entToComp[last_ent.id] = ent_comp_idx;
		}
		static T& get(ent_type e) {
			return _comps[_entToComp[e.id]];
		}
		static int size() { return _comps.size(); }
		static T& get(index_type idx) {
			return _comps[idx];
		}
		static ent_type entity(index_type idx) {
			return _compToEnt[idx];
		}
	private:
		static inline Bag<T,Params.InitialPackedSize>			_comps;
		static inline Bag<index_type,Params.InitialEntities>	_entToComp;
		static inline Bag<ent_type,Params.InitialPackedSize>	_compToEnt;

		static inline StorageCallbacks callbacks{del};

		__attribute__((used))
		static inline StorageRegister<T> reg{callbacks};
	};
	template <class T>
	class TaggedStorage final : NoInstance
	{
	public:
		static void add(ent_type, const T&) {}
		static void del(ent_type) {}
		static T& get(ent_type) = delete;
	};

	template <class T>
	struct Storage final : NoInstance {
		using type = SparseStorage<T>;
	};

	class SingleMask final
	{
	public:
		using bit_type = mask_type;
		static constexpr bit_type bit(index_type idx) { return 1<<idx; }

		void set(const bit_type b) { _mask |= b; }

		void clear(const bit_type b) { _mask &= ~b; }
		void clear() { _mask = 0; }

		bool test(const bit_type b) const { return _mask & b; }
		bool test(const SingleMask m) const { return (_mask & m._mask) == m._mask; }

		index_type ctz() const { return _mask ? __builtin_ctz(_mask) : -1; }
	private:
		mask_type	_mask{0};
	};
	class MultiMask final
	{
	public:
		using bit_type = struct {
			const index_type	index;
			const mask_type		mask;
		};
		static constexpr bit_type bit(index_type idx) {
			return {idx/BitsetWidth, static_cast<mask_type>(1<<(idx%BitsetWidth))};
		}

		void set(const bit_type& b) { _masks[b.index] |= b.mask; }

		void clear(const bit_type& b) { _masks[b.index] &= ~b.mask; }
		void clear() { memset(_masks, 0, sizeof(_masks)); }

		bool test(const bit_type& b) const { return _masks[b.index] & b.mask; }
		bool test(const MultiMask& m) const {
			for (index_type i = 0; i < Size; ++i)
				if ((_masks[i] & m._masks[i]) != m._masks[i])
					return false;
			return true;
		}

		index_type ctz() const {
			for (index_type i = 0; i < Size; ++i) {
				if (_masks[i]) {
					int c = __builtin_ctz(_masks[i]);
					return c + i*BitsetWidth;
				}
			}
			return -1;
		}
	private:
		static constexpr size_type	Size = (Params.MaxComponents-1)/BitsetWidth + 1;
		mask_type					_masks[Size] ={};
	};
	using Mask = std::conditional_t<Params.MaxComponents<=BitsetWidth, SingleMask, MultiMask>;

	static inline index_type compCounter = -1;
	template <class>
	struct Component final : NoInstance
	{
		static inline const index_type		Index = ++compCounter;
		static inline const Mask::bit_type	Bit = Mask::bit(Index);
	};

	struct AddedMask {
		Mask prev;
		Mask next;
		ent_type e;
	};

	class World final : NoInstance
	{
	public:
		static ent_type createEntity() {
			if (_ids.size() > 0)
				return _ids.pop();
			_masks.push(Mask{});
			return {++_maxId.id};
		}
		static void destroyEntity(ent_type ent) {
			if constexpr (Params.CallbackOnDestroy) {
				Mask m = _masks[ent.id];
				int ctz = m.ctz(); // count-trailing-zeros
				while (ctz >= 0) {
					if (_callbacks[ctz].destroy != nullptr)
						_callbacks->destroy(ent);
					m.clear(Mask::bit(ctz));
					ctz = m.ctz();
				}
			}
			_masks[ent.id].clear();
			_ids.push(ent);
		}
		static const Mask& mask(ent_type e) {
			return _masks[e.id];
		}
		static ent_type maxId() { return _maxId; }

		template <class T>
		static T& getComponent(ent_type e) {
			return Storage<T>::type::get(e);
		}

		template <class T>
		static void addComponent(ent_type e, const T& t) {
			Mask prev = _masks[e.id];

			_masks[e.id].set(Component<T>::Bit);
			Storage<T>::type::add(e,t);

			if constexpr (Params.AggregateUpdates) {
				Mask next = _masks[e.id];
				_added.push({prev,next,e});
			}
		}
		template <class T, class...Ts>
		static void addComponents(ent_type e, const T& t, const Ts&... ts) {
			addComponent(e, t);
			if constexpr (sizeof...(Ts)>0)
				addComponents(e, ts...);
		}

		template <class T>
		static void delComponent(ent_type e) {
			_masks[e.id].clear(Component<T>::Bit);
			Storage<T>::type::del(e);
		}
		template <class T, class ...Ts>
		static void delComponents(ent_type e) {
			delComponent<T>(e);
			if constexpr (sizeof...(Ts)>0)
				delComponents<Ts...>(e);
		}

		template <class T>
		static void registerStorage(StorageCallbacks& cb) {
			_callbacks[Component<T>::Index] = cb;
		}

		static size_type sizeAdded() { return _added.size(); }
		static const AddedMask& getAdded(int i) { return _added[i]; }

		static void step() { _added.clear(); }
	private:
		static inline StorageCallbacks _callbacks[Params.MaxComponents] = {nullptr};
		static inline Bag<AddedMask,Params.IdBagSize>		_added;

		static inline ent_type								_maxId{-1};
		static inline Bag<Mask,		Params.InitialEntities> _masks;
		static inline Bag<ent_type,	Params.IdBagSize>		_ids;
	};

	template <class T>
	class StorageRegister
	{
	public:
		StorageRegister(StorageCallbacks& cb) {
			World::registerStorage<T>(cb);
		}
	};

	class Entity
	{
	public:
		Entity(ent_type e) : _ent(e) {}
		ent_type entity() const { return _ent; }

		static Entity create() { return World::createEntity(); }
		void destroy() const { World::destroyEntity(_ent); }

		const Mask& mask() const { return World::mask(_ent); }

		template <class T> T& get() const { return World::getComponent<T>(_ent); }
		template <class T> void add(const T& t) const {
			return World::addComponent<T>(_ent, t);
		}
		template <class T> void del() const {
			return World::delComponent<T>(_ent);
		}

		template <class T, class ...Ts> void addAll(const T& t, const Ts&... ts) const {
			World::addComponents(_ent, t, ts...);
		}
		template <class T, class ...Ts> void delAll() const {
			World::delComponents<T,Ts...>(_ent);
		}

		template <class T> bool has() const { return mask().test(Component<T>::Bit); }
		bool test(const Mask& m) const { return mask().test(m); }
	private:
		ent_type _ent;
	};

	class MaskBuilder
	{
	public:
		template <class T>
		MaskBuilder& set() {
			m.set(Component<T>::Bit);
			return *this;
		}
		Mask build() const { return m; }
	private:
		Mask m;
	};
}
