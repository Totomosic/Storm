import struct
import pytorch_lightning as pl
import torch
from torch import nn
import torch.nn.functional as F
from constants import INPUT_NEURONS, HIDDEN_NEURONS, OUTPUT_NEURONS, FLOAT_SIZE

def filter_string(data: str) -> str:
    return data.replace("\t", "").replace("\n", "").replace("\r", "").replace(" ", "")

class NNUEModel(pl.LightningModule):
    def __init__(self, learning_rate: float, device):
        super(NNUEModel, self).__init__()
        self.learning_rate = learning_rate
        self.model = nn.Sequential(
            nn.Linear(INPUT_NEURONS, HIDDEN_NEURONS),
            nn.ReLU(),
            nn.Linear(HIDDEN_NEURONS, OUTPUT_NEURONS)
        ).float()
        self.model = self.model.to(device)

    def set_from_file(self, filename: str):
        with open(filename, "r") as f:
            data = f.read()
        start_index = data.index("{")
        end_index = data.index("}", start_index + 1)
        if start_index >= 0 and end_index >= 0:
            byte_string = filter_string(data[start_index + 1 : end_index].strip())
            byte_data = bytearray(map(lambda v: int(v), byte_string.split(",")))
            float_count = len(byte_data) // FLOAT_SIZE
            floats = struct.unpack("f" * float_count, byte_data)

            assert len(floats) == INPUT_NEURONS * HIDDEN_NEURONS + HIDDEN_NEURONS + HIDDEN_NEURONS + OUTPUT_NEURONS

            index = 0
            # Hidden biases
            for i in range(HIDDEN_NEURONS):
                self.model[0].bias.data[i] = float(floats[index])
                index += 1
            # Hidden Weights
            for i in range(INPUT_NEURONS):
                for j in range(HIDDEN_NEURONS):
                    self.model[0].weight.data[j][i] = float(floats[index])
                    index += 1
            # Output Bias
            self.model[2].bias.data[0] = float(floats[index])
            index += 1
            # Ouput Weights
            for i in range(HIDDEN_NEURONS):
                self.model[2].weight.data[0][i] = float(floats[index])
                index += 1

            return True
        else:
            print("Invalid input file")
        return False

    def save_to_file(self, filename: str, should_format: bool = True):
        floats = []
        # Hidden biases
        for i in range(HIDDEN_NEURONS):
            floats.append(self.model[0].bias.data[i])
        # Hidden Weights
        for i in range(INPUT_NEURONS):
            for j in range(HIDDEN_NEURONS):
                floats.append(self.model[0].weight.data[j][i])
        # Output Bias
        floats.append(self.model[2].bias.data[0])
        # Ouput Weights
        for i in range(HIDDEN_NEURONS):
            floats.append(self.model[2].weight.data[0][i])
        byte_data = struct.pack("f" * len(floats), *floats)
        string_data = "unsigned char label[] = {" + "\n" if should_format else ""
        for idx, b in enumerate(byte_data):
            new_line = should_format and (idx + 1) % 20 == 0
            string_data += "{}{},{}".format("\t" if should_format and idx % 20 == 0 else "", int(b), "\n" if new_line else "")
        string_data = string_data[:string_data.rfind(",")] + "\n}" if should_format and idx % 20 != 0 else "}"
        string_data += ";\n"
        with open(filename, "w") as f:
            f.write(string_data)

    def forward(self, inputs):
        return self.model(inputs)

    def _step(self, batch, batch_index, step_type: str):
        x, y = batch
        prediction = self.model(x)
        loss = F.mse_loss(prediction.flatten(), y.float()).float()
        self.log("{}_loss".format(step_type), loss.item())
        return loss

    def training_step(self, batch, batch_index):
        return self._step(batch, batch_index, "train")

    def validation_step(self, batch, batch_index):
        return self._step(batch, batch_index, "validation")

    def configure_optimizers(self):
        return torch.optim.SGD(self.model.parameters(), lr=self.learning_rate)
