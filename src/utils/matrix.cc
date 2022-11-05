#include "matrix.h"
#include <iostream>
#include <iomanip>

void Matrix::copy_data(SPARSE_MATRIX &data) const {
	data.clear();
	for (auto &d: m_matrix) {
		data[d.first] = d.second;
	}
}

Matrix::Matrix() {}
Matrix::Matrix(size_t a_cols, size_t a_rows): m_rows(a_rows), m_cols(a_cols) {}
Matrix::Matrix(size_t a_row_cols): m_rows(a_row_cols), m_cols(a_row_cols) {};
Matrix::Matrix(const Matrix &other) {
	m_rows = other.rows();
	m_cols = other.cols();
	other.copy_data(m_matrix);
}

size_t Matrix::rows() const {return m_rows; }
size_t Matrix::cols() const {return m_cols; }

Matrix::Matrix(const ROW & a_row): m_rows(1), m_cols(a_row.size()) {
	int i=0;
	for (auto d: a_row) {
		if (d) m_matrix[id(i++, 0)] = d; else i++;
	}
}

Matrix::Matrix(const std::initializer_list<ROW> a_rows) {
	m_rows = a_rows.size();
	m_cols = 0;
	for (auto row: a_rows)
		if (m_cols < row.size()) m_cols = row.size();
	if (!m_cols) throw std::string("Invalid matrix initialisation data");

	int j = 0;
	for (auto row: a_rows) {
		int i = 0;
		for (auto d: row)
			if (d) m_matrix[id(i++, j)] = d; else i++;
		j++;
	}
}

const Matrix::dProxy Matrix::operator() (size_t i, size_t j) const {
	size_t l_id = id(i, j);
	return dProxy(m_matrix, l_id);
}

Matrix::dProxy Matrix::operator() (size_t i, size_t j) {
	size_t l_id = id(i, j);
	return dProxy(m_matrix, l_id);
}

void Matrix::view() const {
	std::cout << std::setfill(' ');
	for (size_t i=0; i<m_cols; ++i) {
		std::cout << ((i==0)?"   ┌──":"──");
		std::cout << "───────";
		std::cout << ((i+1==m_cols)?"┐":"┬");
	}
	std::cout << "\n";

	for (size_t j=0; j<m_rows; ++j) {
		std::cout << " ";
		for (size_t i=0; i<m_cols; ++i) {
			std::cout << (i==0?"  │ ":" ┊ ") << std::setprecision(2) << std::setw(7) << (*this)(i,j);
		}
		std::cout << " │";
		std::cout << std::endl;
		if (j+1 < m_rows) {
			for (size_t i=0; i<m_cols; ++i) {
				std::cout << ((i==0)?"   ├┄┄":"┄┄");
				std::cout << "┄┄┄┄┄┄┄";
				std::cout << ((i+1==m_cols)?"┤":"┼");
			}
			std::cout << "\n";
		}
	}
	std::cout << " ";
	for (size_t i=0; i<m_cols; ++i) {
		std::cout << ((i==0)?"  └──":"──");
		std::cout << "───────";
		std::cout << ((i+1==m_cols)?"┘":"┴");
	}
	std::cout << "\n";
}

double Matrix::reduce(size_t a_fix_i, size_t a_fix_j) const {
	Matrix m(m_cols-1, m_rows-1);
//	std::cout << "reducing (i,j)=[" << a_fix_i << "," << a_fix_j << "]\n";
	for (size_t j=0; j < m_rows-1; ++j) {
		for (size_t i=0; i< m_cols-1; ++i) {
			size_t di = (i >= a_fix_i) ? i+1:i;
			size_t dj = (j >= a_fix_j) ? j+1:j;
			m(i,j) = (*this)(di, dj);
		}
	}
	double determinant = m.determinant();
//	std::cout << "determinant of \n";
//	m.view();
//	std::cout << " is " << determinant << std::endl;
	return determinant;
}

bool Matrix::is_square() const { return m_rows == m_cols; }


void Matrix::sign_terms() {
	bool row_add = true;
	Matrix &m = *this;
	for (size_t j=0; j < m_rows; ++j) {
		bool add_col = row_add;
		row_add = not row_add;
		for (size_t i=0; i< m_cols; ++i) {
			if (not add_col)
				m(i,j) = -m(i,j);
			add_col = not add_col;
		}
	}
}

double Matrix::add_terms() const {
	double total = 0;
	const Matrix &m = *this;
	for (size_t j=0; j < m_rows; ++j) {
		for (size_t i=0; i< m_cols; ++i) {
			total = total + m(i,j);
		}
	}
	return total;
}

double Matrix::sign_and_add_terms() const {
	bool row_add = true;
	double total = 0;
	const Matrix &m = *this;
	for (size_t j=0; j < m_rows; ++j) {
		bool add_col = row_add;
		row_add = not row_add;
		for (size_t i=0; i< m_cols; ++i) {
			total = add_col?total + m(i,j):total - m(i,j);
			add_col = not add_col;
		}
	}
	return total;
}

double Matrix::determinant() const {
	if (!is_square())
		throw std::string("Attempting to return the determinant of a non-square matrix");
	const Matrix &m = *this;
	if (m_cols == 2)
		return m(0,0) * m(1,1) - m(1,0) * m(0,1);
	else if (m_cols == 1)
		return m(0,0);
	else if (m_cols == 0)
		return 0;

	Matrix n(m_cols, 1);
	for (size_t i=0; i< m_cols; ++i)
		n(i,0) = m(i,0) * m.reduce(i,0);

	return n.sign_and_add_terms();
}

Matrix Matrix::minors() const {
	if (!is_square())
		throw std::string("Attempting to return the adjugate of a non-square matrix");
	Matrix m(m_cols, m_rows);

	for (size_t j=0; j < m_rows; ++j) {
		for (size_t i=0; i< m_cols; ++i) {
			m(i,j) = reduce(i,j);      // Minors
		}
	}
	return m;
}

Matrix Matrix::cofactors() const {
	if (!is_square())
		throw std::string("Attempting to return the adjugate of a non-square matrix");
	Matrix m = minors();
	m.sign_terms();
	return m;
}

Matrix Matrix::adjunct() const {
	if (!is_square())
		throw std::string("Attempting to return the adjunct of a non-square matrix");
	Matrix m = cofactors();
	return m.transpose();
}

Matrix Matrix::adjunct_and_determinant(double &determinant) const {
	if (!is_square())
		throw std::string("Attempting to return the adjunct of a non-square matrix");
	Matrix m = cofactors();
	Matrix n(m_cols, 1);
	for (size_t i=0; i < m_cols; ++i)
		n(i,0) = m(i,0) * (*this)(i,0);
	determinant = n.add_terms();
	return m.transpose();
}

Matrix Matrix::transpose() const {
	Matrix m(m_rows, m_cols);
	for (size_t i=0; i< m_cols; ++i)
		for (size_t j=0; j < m_rows; ++j) {
			double v = (*this)(i,j);
			if (v) m(j,i) = v;
		}
	return m;
}

void Matrix::multiply(double v) {
	Matrix &m = *this;
	for (size_t i=0; i< m_cols; ++i)
		for (size_t j=0; j < m_rows; ++j)
			m(i,j) = m(i,j) * v;
}

Matrix Matrix::multiply(double v) const {
	const Matrix m = (*this);
	m.multiply(v);
	return m;
}

Matrix Matrix::multiply(const Matrix &other) const {
	const Matrix m = (*this);
	if (m.cols() != other.rows())
		throw std::string("Cannot multiply matrices with inappropriate dimensions");
	Matrix n(other.cols(), m.rows());
	for (size_t j=0; j < m.rows(); ++j) {
		for (size_t i=0; i<other.cols(); ++i) {
			double dot = 0;
			for (size_t x=0; x < m_cols; ++x)
				dot += m(x,j) * other(i,x);
			n(i, j) = dot;
		}
	}
	return n;
}

Matrix Matrix::invert() const {
	if (!is_square())
		throw std::string("Attempting to return the inverse of a non-square matrix");
	double det;
	Matrix m = adjunct_and_determinant(det);
	std::cout << "determinant is " << det << std::endl;
	if (det == 0)
		throw std::string("Non-invertable matrix");
	m.multiply(1/det);
	return m;
}

// #define __test__matrix__
#ifdef __test__matrix__
int main() {
	Matrix m({{1,0,0,1},{0,2,1,2},{2,1,0,1},{2,0,1,4}});

	const Matrix a = m.adjunct();
	std::cout << "matrix\n";
	m.view();
	std::cout << "adjoint\n";
	a.view();
	Matrix i = m.invert();
	std::cout << "inverse\n";
	i.view();

	Matrix id = m.multiply(i);
	std::cout << "identity\n";
	id.view();

	Matrix p({3, 4, 2});
	Matrix q({{13, 9,  7, 15},
			  { 8, 7,  4,  6},
			  { 6, 4,  0,  3}});

	std::cout << "p\n";
	p.view();
	std::cout << "q\n";
	q.view();

	Matrix z = p.multiply(q);
	std::cout << "p * q\n";
	z.view();
}
#endif
