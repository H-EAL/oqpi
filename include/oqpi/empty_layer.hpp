#pragma once

namespace oqpi {

	// Determines if two templates are the same
	template<template<typename> typename T1, template<typename> typename T2>
	struct is_same_template
		: public std::false_type
	{};
	// Same!
	template<template<typename> typename T>
	struct is_same_template<T, T>
		: public std::true_type
	{};


	// Defines an empty layer for augmented interfaces
	template<typename>
	struct empty_layer {};

	// Determines if a given layer is empty
	template<template<typename> typename _Layer>
	struct is_empty_layer
		: public is_same_template<_Layer, empty_layer>
	{};

	// Shortcut removing the necessity to call ::value from is_empty_layer
	template<typename _Layer>
	static constexpr auto is_empty_layer_v = is_empty_layer<_Layer>::value;

} /*oqpi*/
