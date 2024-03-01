#include "Agent.hpp"
#include "ve_log.hpp"
#include <ATen/ops/relu.h>
#include <ATen/ops/sigmoid.h>
#include <random>
#include <torch/csrc/autograd/anomaly_mode.h>
#include <torch/serialize.h>

#define GAMMA 0.999

Agent::Agent(bool train_mode) : nn(std::make_shared<NeuralNet>()), train_mode(train_mode)
{
    nn->train(train_mode);
    optimizer = std::make_unique<torch::optim::Adam>(nn->parameters(), 0.001);
}

Agent::Action Agent::get_action(const std::vector<float>& state)
{
    torch::Tensor t = torch::tensor(state);
    torch::Tensor actions_t = nn->forward(t);
    std::discrete_distribution<uint32_t> dis(actions_t.const_data_ptr<float>(), actions_t.const_data_ptr<float>() + actions_t.size(0));
    uint32_t action_index = dis(gen);
    actions.push_back(index_to_action_mask(action_index));
    action_tensors.push_back(actions_t.unsqueeze(0));
    action_log_probs.push_back(torch::log2(actions_t[action_index].unsqueeze(0)));
    return actions.back();
}

void Agent::add_reward_for_last_action(float reward)
{
    rewards.push_back(reward);
}

void Agent::optimize()
{
    VE_ASSERT(rewards.size() == action_log_probs.size(), "Failed to optimize agent: different number of rewards ({}) and actions ({})!", rewards.size(), action_log_probs.size());
    if (rewards.size() <= 2)
    {
        clear_buffers();
        return;
    }
    // Calculate discounted rewards
    float c = 0;
    for (auto r : rewards) c += r;
    std::cout << "Total Reward: " << c << std::endl;
    float running_add = 0.0f;
    std::vector<float> discounted_rewards(rewards.size(), 0.0f);
    for (int i = rewards.size() - 1; i >= 0; --i)
    {
        running_add = running_add * GAMMA + rewards[i];
        discounted_rewards[i] = running_add;
    }

    // Normalize the discounted rewards
    torch::Tensor returns = torch::tensor(discounted_rewards, torch::requires_grad()).unsqueeze(1);
    returns = (returns - returns.mean()) / (returns.std() + 1e-8);
    std::vector<torch::Tensor> policy_loss;
    for (uint32_t i = 0; i < action_log_probs.size(); ++i)
    {
        policy_loss.push_back(-action_log_probs[i] * returns[i]);
    }

    optimizer->zero_grad();
    torch::Tensor loss = torch::cat(policy_loss).sum();
    loss.backward();
    optimizer->step();
    clear_buffers();
}

bool Agent::is_training()
{
    return train_mode;
}

void Agent::save_to_file(const std::string& filename)
{
    torch::save(nn, filename);
}

void Agent::load_from_file(const std::string& filename)
{
    torch::load(nn, filename);
}

void Agent::clear_buffers()
{
    action_tensors.clear();
    rewards.clear();
    action_log_probs.clear();
}

