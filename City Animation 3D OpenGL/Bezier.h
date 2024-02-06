#pragma once

#include <vector>
using namespace std;

#define SIZE 3

struct Vector3
{
    double x, y, z;

    Vector3(double x, double y, double z) : x(x), y(y), z(z) {}

    Vector3 ComputeCrossProduct(Vector3 v) const
    {
        double X, Y, Z;

        X = y * v.z - z * v.y;
        Y = z * v.x - x * v.z;
        Z = x * v.y - y * v.x;

        return Vector3(X, Y, Z);
    }
};

class BezierSurface
{
private:
    int Binomials[SIZE + 1] = {};
    double BaseZValues[SIZE + 1][SIZE + 1] = {};

public:
    BezierSurface()
    {
        for (int i = 0; i <= SIZE; ++i)
        {
            Binomials[i] = ComputeBinomCoeff(SIZE, i);
        }
    }

    void SetBaseZValues(float* zValues)
    {
        int k = 0;
        for (int i = 0; i <= SIZE; ++i)
        {
            for (int j = 0; j <= SIZE; ++j)
            {
                BaseZValues[i][j] = zValues[k++];
            }
        }
    }

    void FillArrayWithValues(float* vertices, int length, int accuracy)
    {
        int k = 0;
        float diff = 1.0f / accuracy;
        float empty = 1.0f;

        for (int i = 0; i < accuracy; ++i)
        {
            for (int j = 0; j < accuracy; ++j)
            {
                float x1 = i * diff, y1 = j * diff;
                float x2 = (i + 1) * diff, y2 = j * diff;
                float x3 = i * diff, y3 = (j + 1) * diff;
                float x4 = (i + 1) * diff, y4 = (j + 1) * diff;

                float z1 = (float)ComputeZValueInPoint(x1, y1);
                float z2 = (float)ComputeZValueInPoint(x2, y2);
                float z3 = (float)ComputeZValueInPoint(x3, y3);
                float z4 = (float)ComputeZValueInPoint(x4, y4);

                Vector3 n1 = ComputeNormalVectorInPoint(x1, y1, z1);
                Vector3 n2 = ComputeNormalVectorInPoint(x2, y2, z2);
                Vector3 n3 = ComputeNormalVectorInPoint(x3, y3, z3);
                Vector3 n4 = ComputeNormalVectorInPoint(x4, y4, z4);

                FillRowWithValues(vertices, k, x1, y1, z1, n1, empty);
                k += 8;

                FillRowWithValues(vertices, k, x2, y2, z2, n2, empty);
                k += 8;

                FillRowWithValues(vertices, k, x3, y3, z3, n3, empty);
                k += 8;

                FillRowWithValues(vertices, k, x3, y3, z3, n3, empty);
                k += 8;

                FillRowWithValues(vertices, k, x2, y2, z2, n2, empty);
                k += 8;

                FillRowWithValues(vertices, k, x4, y4, z4, n4, empty);
                k += 8;
            }
        }
    }

private:
    void FillRowWithValues(float* vertices, int startIndex, float x, float y, float z, Vector3 n, float empty)
    {
        int k = startIndex;
        vertices[k] = x; vertices[k + 1] = y; vertices[k + 2] = z;
        vertices[k + 3] = (float)n.x; vertices[k + 4] = (float)n.y; vertices[k + 5] = (float)n.z;
        vertices[k + 6] = empty; vertices[k + 7] = empty;
    }

    double ComputeZValueInPoint(double x, double y)
    {
        double z = 0;

        for (int i = 0; i <= SIZE; ++i)
        {
            for (int j = 0; j <= SIZE; ++j)
            {
                z += BaseZValues[i][j] * ComputeBernstein(i, x) * ComputeBernstein(j, y);
            }
        }

        return z;
    }

    Vector3 ComputeNormalVectorInPoint(double x, double y, double z)
    {
        Vector3 U = ComputePartialDerivativeX(x, y);
        Vector3 V = ComputePartialDerivativeY(x, y);
        return U.ComputeCrossProduct(V);
    }

	int ComputeBinomCoeff(int n, int k)
	{
        if (k > n)
        {
            return 0;
        }
        if (k == n)
        {
            return 1;
        }
        if (k > n - k)
        {
            k = n - k;
        }

        int c = 1;
        for (int i = 1; i <= k; ++i)
        {
            c *= n--;
            c /= i;
        }

        return c;
	}

    double ComputeBernstein(int i, double t) const
    {
        return Binomials[i] * pow(t, i) * pow(1 - t, SIZE - i);
    }

    double ComputeBernsteinDerivative(int i, double t)
    {
        if (i == 0)
        {
            return Binomials[0] * pow(1 - t, SIZE - 1) * SIZE * (-1);
        }

        if (i == SIZE)
        {
            return Binomials[SIZE] * pow(t, SIZE - 1) * SIZE;
        }

        return Binomials[i] * pow(t, i - 1) * pow(1 - t, SIZE - i - 1) * (SIZE * t - i) * (-1);
    }

    Vector3 ComputePartialDerivativeX(double x, double y)
    {
        double dx, dy, dz;

        dx = 1;
        dy = 0;
        dz = 0;
        for (int i = 0; i <= SIZE; ++i)
        {
            for (int j = 0; j <= SIZE; ++j)
            {
                dz += BaseZValues[i][j] * ComputeBernsteinDerivative(i, x) * ComputeBernstein(j, y);
            }
        }

        return Vector3(dx, dy, dz);
    }

    Vector3 ComputePartialDerivativeY(double x, double y)
    {
        double dx, dy, dz;

        dx = 0;
        dy = 1;
        dz = 0;
        for (int i = 0; i <= SIZE; ++i)
        {
            for (int j = 0; j <= SIZE; ++j)
            {
                dz += BaseZValues[i][j] * ComputeBernstein(i, x) * ComputeBernsteinDerivative(j, y);
            }
        }

        return Vector3(dx, dy, dz);
    }
};