#include "NeuralNet.hpp"
#include <ATen/ops/softmax.h>
#include <torch/serialize/input-archive.h>

NeuralNet::NeuralNet()
{
    fc0 = register_module("fc0", torch::nn::Linear(8, 32));
    fc1 = register_module("fc1", torch::nn::Linear(32, 32));
    fc2 = register_module("fc2", torch::nn::Linear(32, 32));
    fc3 = register_module("fc3", torch::nn::Linear(32, 32));
    fc4 = register_module("fc4", torch::nn::Linear(32, 32));
    fc5 = register_module("fc5", torch::nn::Linear(32, 11));
}

torch::Tensor NeuralNet::forward(torch::Tensor x)
{
    x = torch::relu(fc0(x));
    x = torch::relu(fc1(x));
    x = torch::relu(fc2(x));
    x = torch::relu(fc3(x));
    x = torch::relu(fc4(x));
    x = torch::softmax(fc5(x), 0);
    return x;
}

void NeuralNet::save_to_file(const std::string& filename)
{
    torch::serialize::OutputArchive output;
    fc0->save(output);
    fc1->save(output);
    fc2->save(output);
    fc3->save(output);
    fc4->save(output);
    fc5->save(output);
    output.save_to(filename);
}

void NeuralNet::load_from_file(const std::string& filename)
{
    torch::serialize::InputArchive input;
    input.load_from(filename);
    fc0->load(input);
    fc1->load(input);
    fc2->load(input);
    fc3->load(input);
    fc4->load(input);
    fc5->load(input);
}

