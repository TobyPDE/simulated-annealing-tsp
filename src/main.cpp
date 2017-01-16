#include "tsp.h"
#include <map>
#include <string>

int main(int argc, const char** argv)
{
    // Set up a random problem instance 
    TSPInstance instance;
    if (argc > 1)
    {
        std::ifstream stream;
        stream.open(argv[1]);
        if(!stream.is_open())
        {
            std::cout << "Cannot open data file.";
            return 1;
        }
        instance.readTSPLIB(stream);
        stream.close();
    }
    else
    {
        instance.createRandom(50);
    }
    instance.calcDistanceMatrix();
    
    // Set up the optimizer 
    Optimizer optimizer;
    
    // Register the moves
    ChainReverseMove move1;
    SwapCityMove move2;
    RotateCityMove move3;
    optimizer.addMove(&move1);
    optimizer.addMove(&move2);
    optimizer.addMove(&move3);
    
    // Register the GUI
    // You can specify the dimensions of the window
    RuntimeGUI gui(750, 750);
    optimizer.addObserver(&gui);
    
    // The time the GUI stops after each iterations. Set to 0 to wait for a 
    // keypress
    gui.waitTime = 7;
    
    // Choose a cooling schedule
    GeometricCoolingSchedule schedule(150, 1e-2, 0.95);
    optimizer.coolingSchedule = &schedule;
    
    // Optimizer loop counts
    optimizer.outerLoops = 100;
    optimizer.innerLoops = 5000;
    // Update the GUI every 2000 iterations
    optimizer.notificationCycle = 1000;
    
    // Run the program
    std::vector<int> result;
    optimizer.optimize(instance, result);
    
    return 0;
}