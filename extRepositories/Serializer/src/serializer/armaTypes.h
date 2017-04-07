#pragma once

namespace serializer {
	template<>
	class Converter<arma::mat> {
	public:
		class Mat {
			uint32_t rows;
			uint32_t cols;
			std::vector<double> values;

		public:

			Mat() {}
			Mat(arma::mat const& _mat) {
				rows = _mat.n_rows;
				cols = _mat.n_cols;
				values.resize(rows * cols);
				auto iter = values.begin();
				for (int r (0); r < rows; ++r) {
					for (int c (0); c < cols; ++c) {
						*iter++ = _mat(r, c);
					}
				}
			}
			arma::mat getMat() const {
				arma::mat mat = arma::zeros(rows, cols);
				auto iter = values.begin();
				for (int r (0); r < rows; ++r) {
					for (int c (0); c < cols; ++c) {
						mat(r, c) = *iter++;
					}
				}
				return mat;
			}
			template <typename Node>
			void serialize(Node& node) {
				node["rows"]  % rows;
				node["cols"]  % cols;
				node["values"] % values;
			}
		};

		template<typename Adapter>
		static void serialize(Adapter& adapter, arma::mat& x) {
			Mat mat(x);
			adapter.serialize(mat);
		}
		template<typename Adapter>
		static void deserialize(Adapter& adapter, arma::mat& x) {
			Mat mat;
			adapter.deserialize(mat);
			x = mat.getMat();
		}
	};

#define SERIALIZER_ARMA_CONVERTER(TYPE) \
	template<> \
	class Converter<TYPE> { \
	public: \
		template<typename Adapter> \
		static void serialize(Adapter& adapter, TYPE& x) { \
			Converter<arma::mat>::serialize(adapter, x); \
		} \
		template<typename Adapter> \
		static void deserialize(Adapter& adapter, TYPE& x) { \
			Converter<arma::mat>::deserialize(adapter, x); \
		} \
	}

SERIALIZER_ARMA_CONVERTER(arma::colvec);
SERIALIZER_ARMA_CONVERTER(arma::colvec2);
SERIALIZER_ARMA_CONVERTER(arma::colvec3);
SERIALIZER_ARMA_CONVERTER(arma::colvec4);
SERIALIZER_ARMA_CONVERTER(arma::colvec5);
SERIALIZER_ARMA_CONVERTER(arma::colvec6);
SERIALIZER_ARMA_CONVERTER(arma::colvec7);
SERIALIZER_ARMA_CONVERTER(arma::colvec8);
SERIALIZER_ARMA_CONVERTER(arma::colvec9);
SERIALIZER_ARMA_CONVERTER(arma::rowvec);
SERIALIZER_ARMA_CONVERTER(arma::rowvec2);
SERIALIZER_ARMA_CONVERTER(arma::rowvec3);
SERIALIZER_ARMA_CONVERTER(arma::rowvec4);
SERIALIZER_ARMA_CONVERTER(arma::rowvec5);
SERIALIZER_ARMA_CONVERTER(arma::rowvec6);
SERIALIZER_ARMA_CONVERTER(arma::rowvec7);
SERIALIZER_ARMA_CONVERTER(arma::rowvec8);
SERIALIZER_ARMA_CONVERTER(arma::rowvec9);
SERIALIZER_ARMA_CONVERTER(arma::mat22);
SERIALIZER_ARMA_CONVERTER(arma::mat33);
SERIALIZER_ARMA_CONVERTER(arma::mat44);
SERIALIZER_ARMA_CONVERTER(arma::mat55);
SERIALIZER_ARMA_CONVERTER(arma::mat66);
SERIALIZER_ARMA_CONVERTER(arma::mat77);
SERIALIZER_ARMA_CONVERTER(arma::mat88);
SERIALIZER_ARMA_CONVERTER(arma::mat99);


}

