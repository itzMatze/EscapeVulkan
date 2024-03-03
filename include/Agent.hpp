#include <random>
#include <torch/optim/adam.h>

#include "NeuralNet.hpp"
#include "MoveActions.hpp"

class Agent
{
public:
    Agent(bool train_mode);
    MoveActionFlags::type get_action(const std::vector<float>& state);
    void add_reward_for_last_action(float reward);
    void optimize();
    bool is_training();
    void save_to_file(const std::string& filename);
    void load_from_file(const std::string& filename);

private:
    std::shared_ptr<NeuralNet> nn;
    std::unique_ptr<torch::optim::Adam> optimizer;
    std::mt19937 gen;
    std::vector<MoveActionFlags::type> actions;
    std::vector<torch::Tensor> action_log_probs;
    std::vector<torch::Tensor> action_tensors;
    std::vector<float> rewards;
    bool train_mode;

    MoveActionFlags::type index_to_action_mask(uint32_t idx);
    void clear_buffers();
};

