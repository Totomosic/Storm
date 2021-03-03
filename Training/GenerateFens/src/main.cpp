#include "Storm.h"
#include <cxxopts.hpp>
#include <random>

#include "ThreadSafeFileWriter.h"
#include "UciEngine.h"
#include "Searcher.h"

using namespace Storm;

static std::string DEFAULT_DEPTH = "8";
static std::string DEFAULT_ITERATIONS = "10000000";
static std::string DEFAULT_OUTPUT_FILE = "fens.txt";

int main(int argc, char** argv)
{
	int maxThreads = std::thread::hardware_concurrency();
	std::string defaultThreads = std::to_string(maxThreads);

	cxxopts::Options options("GenerateFens", "Generate labelled NNUE data");
	options.add_options()
		("d,depth", "Depth to use", cxxopts::value<int>()->default_value(DEFAULT_DEPTH))
		("i,iterations", "Number of positions to generate", cxxopts::value<int64_t>()->default_value(DEFAULT_ITERATIONS))
		("t,threads", "Number of threads to use", cxxopts::value<int>()->default_value(defaultThreads))
		("c,command", "Command to run UCI compatible engine (Required)", cxxopts::value<std::string>())
		("o,output_file", "File to output to", cxxopts::value<std::string>()->default_value(DEFAULT_OUTPUT_FILE))
		("v,verbose", "Verbose mode")
		("h,help", "Print usage");

	cxxopts::ParseResult args = options.parse(argc, argv);

	if (args.count("help"))
	{
		std::cout << options.help() << std::endl;
		std::exit(0);
	}
	if (args.count("command") == 0)
	{
		std::cout << options.help() << std::endl;
		std::cout << "--command is required" << std::endl;
		std::exit(1);
	}

	int depth = std::clamp(args["depth"].as<int>(), 1, 100);
	int iterations = std::max<int64_t>(args["iterations"].as<int64_t>(), 1);
	int threads = std::clamp(args["threads"].as<int>(), 1, maxThreads);
	std::string command = args["command"].as<std::string>();
	std::string outputFile = args["output_file"].as<std::string>();
	bool verbose = args["verbose"].as<bool>();

	float randomMoveChance = 0.5f;

	std::cout << "GenerateFens Options:" << std::endl;
	std::cout << "\tdepth = " << depth << std::endl;
	std::cout << "\titerations = " << iterations << std::endl;
	std::cout << "\tthreads = " << threads << std::endl;
	std::cout << "\tcommand = \"" << command << '"' << std::endl;
	std::cout << "\toutput_file = \"" << outputFile << '"' << std::endl;

	Init();

	ThreadSafeFileWriter writer(outputFile);
	int64_t iterationsPerThread = iterations / threads;
	int64_t delta = iterations - iterationsPerThread * threads;

	SharedData data = { iterations, 0 };

	std::vector<std::thread> threadVector;
	for (int i = 0; i < threads - 1; i++)
	{
		threadVector.push_back(std::thread([&]()
		{
			Searcher searcher(command, writer, data, iterationsPerThread, depth, std::hash<std::thread::id>{}(std::this_thread::get_id()));
			searcher.SetBestMoveChance(0.65f);
			searcher.Start(CreateStartingPosition());
		}));
	}

	std::chrono::time_point startTime = std::chrono::high_resolution_clock::now();

	Searcher searcher(command, writer, data, iterationsPerThread + delta, depth, std::hash<std::thread::id>{}(std::this_thread::get_id()));
	searcher.SetBestMoveChance(0.65f);
	searcher.Start(CreateStartingPosition());

	for (auto& thread : threadVector)
		thread.join();

	std::cout << "Generated " << data.CompletedIterations << " in " << (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - startTime).count()) << " seconds." << std::endl;

	return 0;
}
