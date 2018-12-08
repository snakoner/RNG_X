#include <iostream>
#include <fstream>
#include <cstdlib>
#include <stdint.h>
#include <unistd.h>
#include <cmath>
#include <ios>
#include "opencv2/opencv.hpp"

#define DIV_FACTOR 4
#define L_BRIGHTNESS 64
#define H_BRIGHTNESS 191
#define CAMERA_WIDTH 1280
#define CAMERA_HEIGHT 720
#define PATH "sts-2.1.2/sts-2.1.2/"
#define BITS_PER_SAMPLE 100000


uint32_t recieve_bits(const cv::Mat & M, std::vector<uint8_t> & bits)
{
    uint32_t extracted = 0;
    for(int i = 0; i < M.rows; i += DIV_FACTOR)
            for (int j = 0; j < M.cols; j += DIV_FACTOR)
                for (int k = 0; k < 3; k++)
                    if ((L_BRIGHTNESS < M.data[i*M.cols*3 + j*3 + k])&& (M.data[i*M.cols*3 + j*3 + k] < H_BRIGHTNESS)){
                        extracted++;
                        bits.push_back(M.data[i*M.cols*3 + j*3 + k]);
                    }
    return extracted;
}


void write_per_batch(const std::vector<uint8_t> & bits, std::string file)
{
    std::ofstream F(file, std::ios::out | std::ios::binary | std::ios::app);
    for (auto it = bits.begin(); it < bits.end(); it += 8)
    {
        uint8_t byte = 0;
        for (int j = 0; j < 8; j++)
            byte += *(it + j) << j;
        F << byte;
    }
}

void permutations(std::vector<uint8_t> & bits, uint32_t shuffleStep)
{
    std::vector<uint8_t> result;
    for(int i = 0; i < shuffleStep; i++)
        for(int j = 0; j < bits.size() / shuffleStep + 1 * (i < bits.size() % shuffleStep); j++)
            result.push_back(bits[j*shuffleStep + i]);
    bits.swap(result);
}

int main(int argc, char** argv)
{
    uint64_t bits_required = strtoull(argv[1], NULL, 10);
    uint64_t permutations_step = (uint64_t)sqrt((double)BITS_PER_SAMPLE); 
    std::string file = argv[2];
    cv::VideoCapture movie;
    movie.open(0);
    movie.set(cv::CAP_PROP_FRAME_WIDTH, CAMERA_WIDTH);
    movie.set(cv::CAP_PROP_FRAME_HEIGHT, CAMERA_HEIGHT);

    std::vector<uint8_t> extractedBits;
    for (size_t i = 0; i < bits_required; i += extractedBits.size())
    {   
        std::vector<uint8_t> data;
        for (size_t j = 0; j < BITS_PER_SAMPLE; j += extractedBits.size()) {
            extractedBits.clear();
            cv::Mat frame;
            movie >> frame;
            recieve_bits(frame, extractedBits);
           
            data.insert(data.end(), extractedBits.begin(), extractedBits.end());

            std::cout << "Bits got from sample: " << extractedBits.size() << std::endl;

            if (frame.empty()) break; 
            imshow("Movie", frame);
            usleep(1000);
             if (cv::waitKey(10) == 27) break;
        }
        data.resize(BITS_PER_SAMPLE);
    
        permutations(data, permutations_step);
        write_per_batch(data, PATH+file);
    }
    movie.release();

    return 0;
}