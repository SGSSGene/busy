#pragma once

namespace serializer {
	template<typename T>
	class Converter<Units::StrongDouble<T>> {
	public:
		template<typename Adapter>
		static void serialize(Adapter& adapter, Units::StrongDouble<T>& x) {
			double v = x.value();
			adapter.serialize(v);
		}
		template<typename Adapter>
		static void deserialize(Adapter& adapter, Units::StrongDouble<T>& x) {
			double v;
			adapter.deserialize(v);
			x = v * Units::StrongDouble<T>::one();
		}
	};
}

