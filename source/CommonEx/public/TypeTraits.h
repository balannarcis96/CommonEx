#pragma once

namespace std
{
	//SFINAE util that checks if T is a fuctor with operator()'s signature == TSignature
	template<typename T, typename TSignature>
	struct is_functor
	{
		template<typename TType, TType> struct is_same_type;
		typedef char TrueType[1];
		typedef char FalseType[2];
		
		template<typename TTarget> static TrueType&		check(is_same_type<TSignature, &TTarget::operator()>*);
		template<typename	     > static FalseType&	check(...);

		static bool const value = sizeof(check<T>(nullptr)) == sizeof(TrueType);
	};
}