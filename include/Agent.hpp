#include <random>
#include <torch/optim/adam.h>
#include <torch/torch.h>

#include "NeuralNet.hpp"

class Agent
{
public:
    enum Action
    {
        MOVE_UP =         0b100000,
        MOVE_RIGHT =      0b010000,
        MOVE_DOWN =       0b001000,
        MOVE_LEFT =       0b000100,
        MOVE_UP_RIGHT =   0b110000,
        MOVE_UP_LEFT =    0b100100,
        MOVE_DOWN_RIGHT = 0b011000,
        MOVE_DOWN_LEFT =  0b001100,
        ROTATE_LEFT =     0b000010,
        ROTATE_RIGHT =    0b000001,
        NO_MOVE =         0b000000,
        ACTION_COUNT = 11
    };

    Agent();
    Action get_action(const std::vector<float>& state);
    void add_reward_for_last_action(float reward);
    void optimize();
    void save_to_file(const std::string& filename);
    void load_from_file(const std::string& filename);
private:
    NeuralNet nn;
    std::unique_ptr<torch::optim::Adam> optimizer;
    std::mt19937 gen;
    std::vector<Action> actions;
    std::vector<torch::Tensor> action_log_probs;
    std::vector<torch::Tensor> action_tensors;
    std::vector<float> rewards;

    Action index_to_action_mask(uint32_t idx)
    {
        switch (idx) {
            case 0: return MOVE_UP;
            case 1: return MOVE_RIGHT;
            case 2: return MOVE_DOWN;
            case 3: return MOVE_LEFT;
            case 4: return MOVE_UP_RIGHT;
            case 5: return MOVE_UP_LEFT;
            case 6: return MOVE_DOWN_RIGHT;
            case 7: return MOVE_DOWN_LEFT;
            case 8: return ROTATE_LEFT;
            case 9: return ROTATE_RIGHT;
            default: return NO_MOVE;
        }
    }

    void clear_buffers();
};

