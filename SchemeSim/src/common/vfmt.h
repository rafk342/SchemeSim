#pragma once
#include <format>
#include <cstring>

namespace impl
{
	template<u32 buff_sz>
	struct vfmt_impl
	{
		template<typename... T>
		static const char* format(std::format_string<T...> fmt, T&&... args)
		{
			thread_local static char tls_buff[buff_sz];
			auto r = std::format_to_n(tls_buff, buff_sz, fmt, std::forward<T>(args)...);
			*r.out = '\0';
			return tls_buff;
		}
	};
}

template<typename... T>
const char* vfmt(std::format_string<T...> fmt, T&&... args) {
	return impl::vfmt_impl<0x100u>::format(fmt, std::forward<T>(args)...);
}

template<typename... T>
const char* vfmt512(std::format_string<T...> fmt, T&&... args) {
	return impl::vfmt_impl<0x200u>::format(fmt, std::forward<T>(args)...);
}

template<typename... T>
const char* vfmt1024(std::format_string<T...> fmt, T&&... args) {
	return impl::vfmt_impl<0x400u>::format(fmt, std::forward<T>(args)...);
}
