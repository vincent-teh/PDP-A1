#include <iostream>
#include <memory>

#include <omp.h>


class Matrix
{
public:
    Matrix(int rows, int cols) : m_rows(rows), m_cols(cols), m_data(std::make_unique<double[]>(rows * cols)) {}

    double& operator()(int r, int c) { return m_data[r * m_cols + c]; }

    void set_all(float num) { for (int i=0; i<m_rows * m_cols; i++) m_data[i] = num;}

    Matrix operator+(const Matrix& other);

    void print() {
        for (int i = 0; i < m_rows; ++i) {
            for (int j = 0; j < m_cols; ++j) {
                std::cout << (*this)(i, j) << " ";
            }
            std::cout << std::endl;
        }
    }

private:
        std::unique_ptr<double[]> m_data;
        int m_rows;
        int m_cols;
};


Matrix Matrix::operator+(const Matrix& other)
{
    if (m_rows != other.m_rows || m_cols != other.m_cols) {
        throw std::invalid_argument("Matrix dimensions must match for addition.");
    }

    Matrix result(m_rows, m_cols);
    const int thread_num = omp_get_max_threads();
    const int total_elements = m_rows * m_cols;
    const int load = total_elements / thread_num;  // Manual load balancing

    #pragma omp parallel
    {
        int thread_id = omp_get_thread_num();
        int start = thread_id * load;
        int end = (thread_id == thread_num - 1) ? total_elements : start + load; // Last thread handles remainder

        for (int i = start; i < end; i++) {
            result.m_data[i] = this->m_data[i] + other.m_data[i];  // Row-major access
        }
    }

    return result;
}


int main(int argc, char const *argv[])
{
    const int row = 10000;
    const int col = 10000;
    Matrix A(row, col);
    Matrix B(row, col);
    A.set_all(1);
    B.set_all(2);
    Matrix C = A + B;
    return 0;
}
