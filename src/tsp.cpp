#include "tsp.h"

////////////////////////////////////////////////////////////////////////////////
/// TSPInstance
////////////////////////////////////////////////////////////////////////////////

void TSPInstance::createRandom(int n)
{
    // We generate cities on a 1000x1000 pixel plane
    std::mt19937 generator({std::random_device{}()});
    std::uniform_real_distribution<float> distribution(0.0f,999.0f);

    for (int i = 0; i < n; i++)
    {
        // Create a random city
        City city = std::make_pair(distribution(generator), distribution(generator));
        // Add the city
        addCity(city);
    }
}

void TSPInstance::readTSPLIB(std::istream & sin)
{
    // Wait for the NODE_COORD_SECTION token
    const std::string startToken = "NODE_COORD_SECTION";
    const std::string endToken = "EOF";

    std::string parser;
    do {
        sin >> parser;
    } while (parser != startToken);

    // Parse the cities
    while (true)
    {
        City city;

        // The first element is the ID of the city or the EOF tag
        sin >> parser;
        if (parser == endToken)
        {
            // We are done
            break;
        }

        // Now come the two coordinates
        sin >> city.first;
        sin >> city.second;
        addCity(city);
    }
}

void TSPInstance::calcDistanceMatrix()
{
    // Get the number of cities
    int n = static_cast<int>(cities.size());

    // Allocate the new one
    distances = Matrix<float>(n,n);

    for (int i = 0; i < n; i++)
    {
        for (int j = i; j < n; j++)
        {
            // The distance matrix is symmetric
            distances(i,j) = dist(cities[i], cities[j]);
            distances(j,i) = dist(cities[i], cities[j]);
        }
    }
}

float TSPInstance::calcTourLength(const std::vector<int> & tour) const
{
    assert(tour.size() == cities.size());
    
    float result = 0.0f;
    // Calculate the length of the chain
    for (size_t i = 0; i < tour.size() - 1; i++)
    {
        result += distances(tour[i], tour[i+1]);
    }
    // Close the loop
    result += distances(tour[tour.size() - 1], tour[0]);
    
    return result;
}

////////////////////////////////////////////////////////////////////////////////
/// Optimizer
////////////////////////////////////////////////////////////////////////////////

void Optimizer::optimize(const TSPInstance& instance, std::vector<int> & result) const
{
    // Get the number of cities
    int n = static_cast<int>(instance.getCities().size());
    
    assert(n > 0);
    // There has to be at least one move for the optimization to work
    assert(moves.size() > 0);
    
    // Set up the runtime configuration
    Config config;
    
    // Set up some initial tour
    config.state.resize(n);
    config.bestState.resize(n);
    for (int i = 0; i < n; i++)
    {
        config.state[i] = i;
    }

    // Shuffle the array randomly
    std::srand ( unsigned ( std::time(0) ) );
    std::random_shuffle(config.state.begin() + 1, config.state.end());
    
    config.energy = instance.calcTourLength(config.state);
    
    config.bestEnergy = config.energy;
    
    config.temp = coolingSchedule->initialTemp();
    
    std::mt19937 g({std::random_device{}()});
    // Set up an initial distribution over the possible moves
    std::uniform_int_distribution<int> moveDist(0,static_cast<int>(moves.size()) - 1);
    // A uniform distribution for the acceptance probability
    std::uniform_real_distribution<float> uniformDist(0.0f,1.0f);
    
    // Set up the mover service 
    Optimizer::MoveService* service = new Optimizer::MoveService(n);
    for (size_t i = 0; i < moves.size(); i++)
    {
        moves[i]->setMoveService(service);
    }
    
    // The current proposal/neighbor
    std::vector<int> proposal;
    
    // A total loop counter for the notification cycle
    int loopCounter = 0;
    
    // Start the optimization
    for (config.outer = 0; config.outer < outerLoops; config.outer++)
    {
        // Determine the next temperature
        config.temp = coolingSchedule->nextTemp(config);
        
        // Simulate the markov chain
        for (config.inner = 0; config.inner < innerLoops; config.inner++)
        {
            proposal = config.state;
            
            // Propose a new neighbor according to some move
            // Choose the move
            int m = moveDist(g);
            moves[m]->propose(proposal);
            
            // Get the energy of the new proposal
            const float energy = instance.calcTourLength(proposal);
            const float delta = energy - config.energy;
            
            // Did we decrease the energy?
            if (delta <= 0)
            {
                // Accept the move
                config.state = proposal;
                config.energy = energy;
            }
            else
            {
                // Accept the proposal with a certain probability
                float u = uniformDist(g);
                if (u <= std::exp(-1/config.temp * delta))
                {
                    config.state = proposal;
                    config.energy = energy;
                }
            }
            
            // Is this better than the best global optimum?
            if (energy < config.bestEnergy)
            {
                // It is
                config.bestEnergy = energy;
                config.bestState = proposal;
            }
            
            // Should we notify the observers?
            if ((loopCounter % notificationCycle) == 0)
            {
                // Yes, we should
                for (size_t i = 0; i < observers.size(); i++)
                {
                    observers[i]->notify(instance, config);
                }
            }
            loopCounter++;
        }
    }
    
    // Unregister the move service
    DELETE_PTR(service);
    for (size_t i = 0; i < moves.size(); i++)
    {
        moves[i]->setMoveService(0);
    }
    
    result = config.bestState;
    
    // Do the final notification
    config.terminated = true;
    config.state = config.bestState;
    config.energy = config.bestEnergy;
    for (size_t i = 0; i < observers.size(); i++)
    {
        observers[i]->notify(instance, config);
    }
}

////////////////////////////////////////////////////////////////////////////////
/// RuntimeGUI
////////////////////////////////////////////////////////////////////////////////

void RuntimeGUI::notify(const TSPInstance & instance, const Optimizer::Config & config)
{
    // The screen is split as follows:
    // 75% points
    // 25% status

    // Clear the gui
    gui = cv::Scalar(0);

    // Get the status marker
    int statusCol = 0.75 * gui.cols;

    // Write the status
    std::stringstream ss;
    ss << "temp = " << config.temp;
    cv::putText(    gui, 
                    ss.str(), 
                    cv::Point(statusCol, 15), 
                    cv::FONT_HERSHEY_PLAIN, 
                    0.9, 
                    cv::Scalar(255,255,255));
    ss.str("");
    ss << "outer = " << config.outer;
    cv::putText(    gui, 
                    ss.str(), 
                    cv::Point(statusCol, 30), 
                    cv::FONT_HERSHEY_PLAIN, 
                    0.9, 
                    cv::Scalar(255,255,255));
    ss.str("");
    ss << "inner = " << config.inner;
    cv::putText(    gui, 
                    ss.str(), 
                    cv::Point(statusCol, 45), 
                    cv::FONT_HERSHEY_PLAIN, 
                    0.9, 
                    cv::Scalar(255,255,255));
    ss.str("");
    ss << "energy = " << config.energy;
    cv::putText(    gui, 
                    ss.str(), 
                    cv::Point(statusCol, 60), 
                    cv::FONT_HERSHEY_PLAIN, 
                    0.9, 
                    cv::Scalar(255,255,255));
    ss.str("");
    ss << "best energy = " << config.bestEnergy;
    cv::putText(    gui, 
                    ss.str(), 
                    cv::Point(statusCol, 75), 
                    cv::FONT_HERSHEY_PLAIN, 
                    0.9, 
                    cv::Scalar(255,255,255));

    // Plot the charts
    // [...]

    // Plot the cities
    // Determine the minimum and maximum X/Y
    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::min();
    float maxY = std::numeric_limits<float>::min();

    for (size_t i = 0; i < instance.getCities().size(); i++)
    {
        minX = std::min(minX, instance.getCities()[i].second);
        minY = std::min(minY, instance.getCities()[i].first);
        maxX = std::max(maxX, instance.getCities()[i].second);
        maxY = std::max(maxY, instance.getCities()[i].first);
    }

    // Calculate the compression factor
    float width = maxX - minX;
    float height = maxY - minY;
    float compression = (statusCol - 10)/width;
    if (height*compression > gui.rows-10)
    {
        compression = (gui.rows-10)/height;
    }

    // Paint the best path
    for (size_t i = 0; i < config.state.size(); i++)
    {
        cv::Point p1;
        p1.x = (instance.getCities()[config.bestState[i%config.state.size()]].second - minX)* compression+5;
        p1.y = (instance.getCities()[config.bestState[i%config.state.size()]].first - minY)* compression+5;
        cv::Point p2;
        p2.x = (instance.getCities()[config.bestState[(i+1)%config.state.size()]].second - minX)* compression+5;
        p2.y = (instance.getCities()[config.bestState[(i+1)%config.state.size()]].first - minY)* compression+5;

        cv::line(gui, p1, p2, cv::Scalar(0,255,255), 1, CV_AA);
    }
    // Paint the current path
    for (size_t i = 0; i < config.state.size(); i++)
    {
        cv::Point p1;
        p1.x = (instance.getCities()[config.state[i%config.state.size()]].second - minX)* compression+5;
        p1.y = (instance.getCities()[config.state[i%config.state.size()]].first - minY)* compression+5;
        cv::Point p2;
        p2.x = (instance.getCities()[config.state[(i+1)%config.state.size()]].second - minX)* compression+5;
        p2.y = (instance.getCities()[config.state[(i+1)%config.state.size()]].first - minY)* compression+5;

        cv::line(gui, p1, p2, cv::Scalar(255,0,255), 2, CV_AA);
    }

    // Paint the cities
    for (size_t i = 0; i < instance.getCities().size(); i++)
    {
        cv::Point p1;
        p1.x = (instance.getCities()[i].second - minX)* compression+5;
        p1.y = (instance.getCities()[i].first - minY)* compression+5;

        cv::circle(gui, p1, 2, cv::Scalar(200,200,200), 2);
    }

    cv::imshow("GUI", gui);
    if (config.terminated)
    {
        cv::waitKey(0);
    }
    else
    {
        cv::waitKey(waitTime);
    }
}