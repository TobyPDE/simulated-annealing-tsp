#ifndef UTIL_H
#define UTIL_H

#include <cstdarg>
#include <cstddef>
#include <stdexcept>
#include <cmath>
#include <cstring>
#include <numeric>
#include <complex>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <valarray>
#include <memory>
#include <algorithm>
#include <functional>
#include <typeinfo>
#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <limits>
#include <cassert>
#include <time.h>
#include <stdio.h>
/**
 * A very simple matrix class
 */
template <class T>
class Matrix
{
private:
    T * a;
    int m, n;

public:
    Matrix() : a(0), m(0), n(0)
    {}

    Matrix(int m, int n) : a(0), m(m), n(n)
    {
        if (n*m) {
                a = new T[n*m];
        }
    }

    /// Copy-Konstruktor
    Matrix(const Matrix& mat) : a(0), m(mat.m), n(mat.n)
    {
        if (n*m)
        {
                a = new T[n*m];
        }
        std::memcpy(a, mat.a, n*m*sizeof(T));
    }

    ~Matrix()
    {
        delete[] a;
    }

    T& operator()(int i, int j)
    {
        assert(i>=0 && i<m && j>=0 && j<n);
        return a[m*j+i];
    }

    T operator()(int i, int j) const
    {
        assert(i>=0 && i<m && j>=0 && j<n);
        return a[m*j+i];
    }

    Matrix& operator=(const Matrix& mat)
    {
        if (mat.n*mat.m != m*n) {
                delete[] a;
                a = new T[mat.m*mat.n];
        }
        m = mat.m; n = mat.n;
        for (int k = 0; k < m*n; k++) {
                a[k] = mat.a[k];
        }
        return *this;
    }

    /// Alle Eintraege auf Wert v setzen
    Matrix& operator=(T v)
    {
        for (int k = 0; k < m*n; k++) {
                a[k] = v;
        }
        return *this;
    }

    Matrix& operator+=(const Matrix& B)
    {
        assert(B.n == n && B.m == m);

        for (int k = 0; k < m*n; k++) {
            a[k] += B.a[k];
        }
        return *this;
    }

    Matrix& operator-=(const Matrix& B)
    {
        assert(B.n == n && B.m == m);

        for (int k = 0; k < m*n; k++) {
            a[k] -= B.a[k];
        }
        return *this;
    }

    Matrix& operator*=(T v)
    {
        for (int k = 0; k < m*n; k++) {
            a[k] *= v;
        }
        return *this;
    }

    Matrix& operator/=(T v)
    {
        for (int k = 0; k < m*n; k++) {
            a[k] /= v;
        }
        return *this;
    }

    void resize(int mm, int nn)
    {
        if (m != mm || n != nn) {
            delete[] a;
            a = new T[mm*nn];
            m = mm;
            n = nn;
        }
    }

    int rows() const
    {
        return m;
    }
    
    int cols() const
    {
        return n;
    }

    friend std::ostream& operator<<(std::ostream& str, const Matrix& M)
    {
        for (int i=0; i<M.m; i++) {
            for (int j=0; j<M.n; j++) {
                if (M(i,j)>=0.)
                {
                    printf(" ");
                    printf("%2.15f ", std::abs(M(i,j)));
                }
                else
                {
                    printf("%2.15f ", M(i,j));
                }
            }
            str << std::endl;
        }
        return str;
    }
};
#endif