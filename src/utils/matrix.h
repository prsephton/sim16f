/*
 * matrix.h
 *
 *  Created on: 27 Oct 2022
 *      Author: paul
 */

#pragma once

#include <cmath>
#include <map>
#include <initializer_list>

class Matrix {
	typedef  std::map<size_t, double> SPARSE_MATRIX;

	struct dProxy {   // prevents assignment of 0 values in m_matrix
		size_t m_key;
		SPARSE_MATRIX &m_matrix;

		dProxy(const SPARSE_MATRIX &a_matrix, size_t a_key):
			m_key(a_key), m_matrix(const_cast<SPARSE_MATRIX &>(a_matrix)) {
		}
		void operator=(double v) {
			if (v) m_matrix[m_key] = v;
			else m_matrix.erase(m_key);
		}
		void operator=(const dProxy &d) {
			double v = (double)d;
			operator =(v);
		}
		operator double() const {
			auto v = m_matrix.find(m_key);
			if (v == m_matrix.end()) return 0;
			return v->second;
		}
//		double operator*(double v) {
//			auto loc = m_matrix.find(m_key);
//			if (loc == m_matrix.end()) return 0;
//			return v * loc->second;
//		}
	};

	size_t m_rows=0, m_cols=0;
	SPARSE_MATRIX m_matrix;

	size_t id(size_t i, size_t j) const {
		return j * m_cols + i;
	}

	typedef std::initializer_list<double> ROW;
  protected:
	void copy_data(SPARSE_MATRIX &data) const;
	void sign_terms();
	double sign_and_add_terms() const;
	double add_terms() const;
	double reduce(size_t a_fix_i, size_t a_fix_j) const;
  public:
	Matrix();
	Matrix(size_t a_row_cols);
	Matrix(size_t a_cols, size_t a_rows);
	Matrix(const Matrix & other);
	Matrix(const ROW & a_row);
	Matrix(const std::initializer_list<ROW> a_rows);

	void view() const;
	size_t rows() const;
	size_t cols() const;

//	double operator() (size_t i, size_t j) const;
	const dProxy operator() (size_t i, size_t j) const;
	dProxy operator() (size_t i, size_t j);

	bool is_square() const;
	double determinant() const;

	void multiply(double v);
	Matrix multiply(double v) const;
	Matrix multiply(const Matrix &other) const;
	Matrix minors() const;
	Matrix cofactors() const;
	Matrix adjunct() const;
	Matrix adjunct_and_determinant(double &determinant) const;
	Matrix invert() const;
	Matrix transpose() const;

};
