int main()
{

    Transmitter tx;
    Receiver rx;
    Noise noise;
    CognitiveEngineList CEList;
    SignalEnvironmentList SEList;

    for(CE in CEList)
    {
        for(SE in SEList)
        {
            tx.initializeDefaults(CE, SE);
            done = false;
            while(!done)
            {
                int N = CE.iterations;
                for(int i= 0; i < N; i++)
                {
                    tx.transmit_packet();
                    noise.add_noise()
                    store_transmitted_packet()
                }
                ///////////////
                rx.receive_packets();
                ///////////////
                //wait for data to come back from rx
                CE.analyze_data();
                if(!CE.optimized())
                    tx.modify_tx_parameters(CE.option_to_adapt, step_value)
                else
                    done = true;
            }
        }
    }
}

// Use libconfig for configuration files

class CognitiveEngine
{
    default_mod_scheme;
    default_tx_power;
    String option_to_adapt;
    String goal;
    String threshold;
    String latestGoalValue;
    int iterations;
}
CognitiveEngine::optimized()
{
    switch goal:
        case "ber": return (float)latestGoalValue < (float)threshold;
        case "per": return 
        case "payload valid": return latestGoalValue == "true";
}
    
