#include <torch/serialize/output-archive.h>
#include <torch/torch.h>

class NeuralNet : public torch::nn::Module
{
public:
    NeuralNet();
    torch::Tensor forward(torch::Tensor x);
    void save_to_file(const std::string& filename);
    void load_from_file(const std::string& filename);
private:
    torch::nn::Linear fc0 = nullptr;
    torch::nn::Linear fc1 = nullptr;
    torch::nn::Linear fc2 = nullptr;
    torch::nn::Linear fc3 = nullptr;
    torch::nn::Linear fc4 = nullptr;
    torch::nn::Linear fc5 = nullptr;
};

