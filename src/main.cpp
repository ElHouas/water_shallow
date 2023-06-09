

#include <math.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <chrono>

// #include <mpi.h>

using namespace std;
using namespace std::chrono;

class ShallowWater {
    public:
    // Field Indices/Parameters
    #define VX          0
    #define VY          1
    #define H           2
    typedef double data_t;
    data_t *** phi;
    data_t *** k1;
    data_t *** k2;
    data_t *** k3;
    data_t *** k4;
    data_t *** tempPhi;
    public:
    //Initialize global variables
    const data_t g = 9.81;        // [ms^-2]
    const data_t x_max = 100;         // [m]
    const data_t y_max = 100;        // [m]
    const data_t t_max = 100;         // [s]
    const data_t N_t = 501;
    const data_t N_x = 101;
    const data_t N_y = 101;
    const data_t Delta_t    = data_t(t_max) / (N_t - 1);
    const data_t Delta_x    = data_t(x_max) / (N_x - 1);
    const data_t Delta_y    = data_t(y_max) / (N_y - 1);
    const data_t Delta_ton2 = Delta_t/2.0;
    const data_t Coef[6]    = {	data_t(-1.0 /60.0), data_t(3.0 /20.0),data_t(-3.0 / 4.0),	data_t(3.0 / 4.0),data_t(-3.0 /20.0),	data_t(1.0 /60.0) }; // CDS6 coefficients;

////////////////////////////////////////////////////////// Allocates memory for a 3D array
    data_t*** new3d(int M, int N, int O)
    {
        data_t*** A = new data_t**[M];
        A[0]        = new data_t* [M*N];
        A[0][0]     = new data_t  [M*N*O];

        for (int m=1, mm=N; m<M; m++, mm+=N)
        {
            A[m] = &A[0][mm];
        }

        for (int mn=1, nn=O; mn<(M*N); mn++, nn+=O)
        {
            A[0][mn] = &A[0][0][nn];
        }

        memset(A[0][0], 0, M*N*O * sizeof(data_t));

        return A;
    }
    void SetParameters(){
        phi      = new3d(3,N_y,N_x); // Contains the three 2D fields v_x, v_y, h
        k1       = new3d(3,N_y,N_x);
        k2       = new3d(3,N_y,N_x);
        k3       = new3d(3,N_y,N_x);
        k4       = new3d(3,N_y,N_x);
        tempPhi  = new3d(3,N_y,N_x);

    }

    void TimeIntegrate(){

        // RK4 time marching loop
        for (size_t nth = 0; nth < N_t-1; nth++)
        {
            f(phi, k1);

            // Compute phi+k1*Delta_t/2
            for (size_t i = 0; i < N_y; i++)
            {
                for(size_t j = 0; j < N_x; j++)
                {
                    for (size_t k = 0; k < 3; k++)
                    {
                        tempPhi[k][i][j] = phi[k][i][j] + k1[k][i][j]*Delta_ton2;
                    }
                }
            }

            f(tempPhi, k2);

            // Compute phi+k2*Delta_t/2
            for (size_t i = 0; i < N_y; i++)
            {
                for(size_t j = 0; j < N_x; j++)
                {
                    for (size_t k = 0; k < 3; k++)
                    {
                        tempPhi[k][i][j] = phi[k][i][j] + k2[k][i][j]*Delta_ton2;
                    }
                }
            }

            f(tempPhi, k3);

            // Compute phi+k3*Delta_t
            for (size_t i = 0; i < N_y; i++)
            {
                for(size_t j = 0; j < N_x; j++)
                {
                    for (size_t k = 0; k < 3; k++)
                    {
                        tempPhi[k][i][j] = phi[k][i][j] + k3[k][i][j]*Delta_t;
                    }
                }
            }

            f(tempPhi, k4);

            // Compute fields for the next time step
            for (size_t i = 0; i < N_y; i++)
            {
                for(size_t j = 0; j < N_x; j++)
                {
                    for (size_t k = 0; k < 3; k++)
                    {
                        phi[k][i][j] = phi[k][i][j] + Delta_t*((1.0/6.0)*k1[k][i][j]+
                                                            (1.0/3.0)*k2[k][i][j]+
                                                            (1.0/3.0)*k3[k][i][j]+
                                                            (1.0/6.0)*k4[k][i][j]);
                    }
                }
            }

            // Save data for the next time step
            save(phi[VX],phi[VY],phi[H], nth+1);
        }
    }
    void DeleteParameters(){
        del3d(phi);
        del3d(k1);
        del3d(k2);
        del3d(k3);
        del3d(k4);
        del3d(tempPhi);
        }


    // Deallocate a 3d array
    void del3d(data_t*** A) {
        if (A[0][0] != nullptr)
            delete[] A[0][0];

        if (A[0] != nullptr)
            delete[] A[0];

        if (A != nullptr)
            delete[] A;
    }

    void SetInitialConditions(){
        // Initialize h
        for (size_t i = 0; i < N_y; i++)
        {
            for (size_t j = 0; j < N_x; j++)
            {
                data_t x = Delta_x*j;
                data_t y = Delta_y*i;
                ////Default
                phi[H][i][j] = 1.0 + 0.5*exp(-((x - 30.0)*(x - 30.0) + (y - 30.0)*(y - 30.0)) / 25.0);
                ////Single Droplet
               //phi[H][i][j] = exp(-((x - 50.0)*(x - 50.0) + (y - 50.0)*(y - 50.0)) / 25.0);

                ////Double Droplet
                //phi[H][i][j] = exp(-((x - 25.0)*(x - 25.0) + (y - 25.0)*(y - 25.0)) / 25.0) + exp(-((x - 75.0)*(x - 75.0) + (y - 75.0)*(y - 75.0)) / 25.0);

               ////Plane wave in X
                //phi[H][i][j] = exp(-((x - 50.0)*(x - 50.0)) / 25.0);

                ////Plane wave in Y
               //phi[H][i][j] = exp(-((y - 50.0)*(y - 50.0)) / 25.0);
            }
        }
    }

    // 2D Spatial derivative function
    void spatial_d(data_t** A, int i, int j, data_t* ddx, data_t* ddy)
    {
        *ddx = 0;
        *ddy = 0;

        // for x derivative
        int index_col[6] = { j - 3 ,   j - 2 ,    j - 1 ,    j + 1  ,   j + 2 ,    j + 3 };

        // for y derivative
        int index_row[6] = { i - 3 ,   i - 2 ,    i - 1 ,    i + 1  ,   i + 2 ,    i + 3 };

        // Since the coefficient at the current index is zero, we do not need to store it into the index array

        for (int k = 0; k < 6; k++)
        {
            // Periodic boundary if indices go out of the bounds of the 2D array
            if (index_col[k] < 0)                   // If exceed right boundary
                index_col[k] += N_x;                // Wrap to left
            else if (index_col[k] > N_x-1)          // If exceed left boundary
                index_col[k] -= N_x;                // Wrap to right

            if (index_row[k] < 0)                   // If exceed bottom boundary
                index_row[k] += N_y;                // Wrap to top
            else if (index_row[k] > N_y-1)          // If exceed top boundary
                index_row[k] -= N_y;                // Wrap to bottom

            *ddx += Coef[k]*A[i][index_col[k]];
            *ddy += Coef[k]*A[index_row[k]][j];
        }

        *ddx = *ddx / Delta_x;
        *ddy = *ddy / Delta_y;
    }

    // Time derivative function
    void f(data_t*** phi, data_t*** k)
    {
        data_t dvxdx;
        data_t dvxdy;
        data_t dvydx;
        data_t dvydy;
        data_t dhdx;
        data_t dhdy;

        // Compute time derivative for all points in grid
        for (size_t i = 0; i < N_y; i++)
        {
            for (size_t j = 0; j < N_x; j++)
            {
                // Calculate spatial derivatives wrt x and y for the three fields
                spatial_d(phi[VX], i, j, &dvxdx, &dvxdy);
                spatial_d(phi[VY], i, j, &dvydx, &dvydy);
                spatial_d(phi[H],  i, j, &dhdx,  &dhdy);

                // The Shallow Water Equation as a coupled ODE
                k[VX][i][j] = -(phi[VX][i][j]*dvxdx + phi[VY][i][j]*dvxdy) - data_t(g*dhdx);
                k[VY][i][j] = -(phi[VX][i][j]*dvydx + phi[VY][i][j]*dvydy) - data_t(g*dhdy);
                k[H][i][j]  = -(phi[VX][i][j]*dhdx  + phi[H][i][j]*dvxdx   + phi[VY][i][j]*dhdy + phi[H][i][j]*dvydy);
            }
        }

        return;
    }

    // Writes data into a file
    void save(data_t** vx,data_t** vy,data_t** h, int nth)
    {
        ofstream myfile;
        char filename[11];
        sprintf(filename,"SWE%d.txt", nth);
        myfile.open(filename);

        for (size_t i = 0; i < N_y; i++)
        {
            for (size_t j = 0; j < N_x; j++)
            {
                data_t x = Delta_x*j;
                data_t y = Delta_y*i;
                myfile << x << " " << y << " " << vx[i][j] << " " << vy[i][j] << " " << h[i][j] << " \n";
            }
        }
        myfile.close();
        std::cout <<"["<< filename << "] saved!" << endl;
    }

};

int main(int argc, char* argv[]){

    ShallowWater W;

    W.SetParameters();
    W.SetInitialConditions();
    // Get starting timepoint
    auto start = high_resolution_clock::now();
    W.TimeIntegrate();
    // Get ending timepoint
    auto stop = high_resolution_clock::now();

    // Get duration in specified time unit
    auto duration = duration_cast<seconds>(stop - start);

    cout << "Time taken by function: " << duration.count() << " seconds" << endl;

    W.DeleteParameters();

    return 0;
}
