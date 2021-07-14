import argparse
from constants import SCORE_MULTIPLIER
import torch
from torch.utils.data import DataLoader
import pytorch_lightning as pl

from model import NNUEModel
from dataloader import PositionsDataset

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--training", type=str, default=None, help="Path to training data")
    parser.add_argument("--validation", type=str, default=None, help="Path to validation data")
    parser.add_argument("--testing", type=str, default=None, help="Path to testing data")
    parser.add_argument("--learning_rate", type=float, default=1e-3, help="Model learning rate")
    parser.add_argument("--epochs", type=int, default=10, help="Number of training epochs")
    parser.add_argument("--output_file", type=str, default="model.net", help="Output model weights filename")
    parser.add_argument("--input_file", type=str, default=None, help="Input model weights file")
    parser = pl.Trainer.add_argparse_args(parser)

    args = parser.parse_args()

    device = "cuda:0" if torch.cuda.is_available() else "cpu"

    model = NNUEModel(args.learning_rate, device)
    if args.input_file:
        model.set_from_file(args.input_file)

    print("Model initialized")

    do_training = args.training is not None and args.validation is not None
    if do_training:
        training_dataset = PositionsDataset(args.training, device)
        validation_dataset = PositionsDataset(args.validation, device)
        training_dataloader = DataLoader(training_dataset, batch_size=4096, shuffle=True)
        validation_dataloader = DataLoader(validation_dataset, batch_size=128, shuffle=False)

        trainer = pl.Trainer.from_argparse_args(args, max_epochs=args.epochs, gpus=1)
        try:
            trainer.fit(model, training_dataloader, validation_dataloader)
        except KeyboardInterrupt:
            pass

    if args.testing:
        try:
            dataset = PositionsDataset(args.testing, "cpu" if do_training else device)
            for i in range(len(dataset)):
                tensor, score = dataset[i]
                predicted_score = model(tensor)
                print("Predicted {} Actual {}".format(int(predicted_score.item() * SCORE_MULTIPLIER), int(score * SCORE_MULTIPLIER)))
        except Exception as e:
            print("TESTING ERROR", e)

    if args.output_file:
        model.save_to_file(args.output_file)
