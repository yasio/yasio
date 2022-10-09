#pragma once
#include <memory>
#include <cstring>
#include <cassert>

namespace xx {

	// 类似 std::vector 的专用于放 pod 或 非严格pod( 是否执行构造，析构 无所谓 ) 类型的容器，可拿走 buf
	// useNewDelete == false: malloc + free
	template<typename T, bool useNewDelete = true>
	struct PodVector {
		T* buf;
		size_t len, cap;

		int realloc_hits = 0;

		PodVector(PodVector const& o) = delete;
		PodVector& operator=(PodVector const& o) = delete;
		explicit PodVector(size_t const& cap_ = 0, size_t const& len_ = 0) : buf(cap_ == 0 ? nullptr : Alloc(cap_)), cap(cap_), len(len_) {}
		~PodVector() {
			if (buf) {
				if constexpr (useNewDelete) {
					delete[](char*)buf;
				}
				else {
					free(buf);
				}
				buf = nullptr;
			}
		}

		PodVector(PodVector&& o) noexcept : buf(o.buf), len(o.len), cap(o.cap) {
			o.buf = nullptr;
			o.len = 0;
			o.cap = 0;
		}
		PodVector& operator=(PodVector&& o) noexcept {
			buf = o.buf;
			len = o.len;
			cap = o.cap;
			o.buf = nullptr;
			o.len = 0;
			o.cap = 0;
		}

		static T* Alloc(size_t const& count) {
			if constexpr (useNewDelete) return (T*)new char[count * sizeof(T)];
			else return (T*)malloc(count * sizeof(T));
		}

		void Reserve(size_t const& cap_) {
			if (cap_ <= cap) return;
			if (!cap) {
				cap = cap_;
			}
			else do {
				cap += cap;
			} while (cap < cap_);

			if constexpr (useNewDelete) {
				auto newBuf = (T*)new char[cap * sizeof(T)];
				memcpy(newBuf, buf, len * sizeof(T));
				delete[](char*)buf;
				buf = newBuf;
                ++realloc_hits;
			}
			else {
				buf = (T*)realloc(buf, cap * sizeof(T));
			}
		}

		void Resize(size_t const& len_) {
			if (len_ > len) {
				Reserve(len_);
			}
			len = len_;
		}

		T* TakeAwayBuf() noexcept {
			auto r = buf;
			buf = nullptr;
			len = 0;
			cap = 0;
			return r;
		}

		template<typename...Args>
		T& Emplace(Args&&...args) noexcept {
			Reserve(len + 1);
			return *new (&buf[len++]) T(std::forward<Args>(args)...);
		}

                T& operator[](size_t idx) { return buf[idx]; }
                const T& operator[](size_t idx) const { return buf[idx]; }
	};
}
