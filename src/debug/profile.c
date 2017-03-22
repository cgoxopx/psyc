/*
 Copyright (c) 2016 Fabio Nicotra.
 All rights reserved.
 
 Redistribution and use in source and binary forms are permitted
 provided that the above copyright notice and this paragraph are
 duplicated in all such forms and that any documentation,
 advertising materials, and other materials related to such
 distribution and use acknowledge that the software was developed
 by the copyright holder. The name of the
 copyright holder may not be used to endorse or promote products derived
 from this software without specific prior written permission.
 THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <stdio.h>
#include <stdlib.h>
#include "../neural.h"
#include "../mnist.h"

#define INPUT_SIZE (28 * 28)
#define EPOCHS 1
#define FEATURES_COUNT 20
#define REGIONS_SIZE 5
#define POOL_SIZE 2
#define TRAIN_DATASET_LEN 2
#define EVAL_DATASET_LEN 1

#define RNN_INPUT_SIZE  4
#define RNN_HIDDEN_SIZE 2
#define RNN_TIMES       4
#define RNN_LEARNING_RATE 0.005

double rnn_train_data[10] = {1, 4, 0, 1, 2, 3, 3, 2, 1, 0};

int main(int argc, char** argv) {
    
    double * test_data = NULL;
    double * train_data = NULL;
    double * eval_data = NULL;
    int datalen = loadMNISTData(TRAINING_DATA,
                                "../../resources/train-images-idx3-ubyte.gz",
                                "../../resources/train-labels-idx1-ubyte.gz",
                                &train_data);
    if (datalen == 0 || train_data == NULL) {
        printf("Could not load training data!\n");
        return 1;
    }
    int testlen = loadMNISTData(TEST_DATA,
                                "../../resources/t10k-images-idx3-ubyte.gz",
                                "../../resources/t10k-labels-idx1-ubyte.gz",
                                &test_data);
    
    NeuralNetwork * network = createNetwork();
    
    LayerParameters * cparams;
    LayerParameters * pparams;
    cparams = createConvolutionalParameters(FEATURES_COUNT, REGIONS_SIZE,
                                            1, 0, 1);
    pparams = createConvolutionalParameters(FEATURES_COUNT, POOL_SIZE,
                                            0, 0, 1);
    addLayer(network, FullyConnected, INPUT_SIZE, NULL);
    addConvolutionalLayer(network, cparams);
    addPoolingLayer(network, pparams);
    addLayer(network, FullyConnected, 30, NULL);
    //addLayer(network, FullyConnected, 10, NULL);
    addLayer(network, SoftMax, 10, NULL);
    
    int element_size = network->input_size + network->output_size;
    datalen = element_size * TRAIN_DATASET_LEN;
    eval_data = train_data + datalen;
    int eval_datalen = element_size * EVAL_DATASET_LEN;
    
    train(network, train_data, datalen, EPOCHS, 1.5, 1, 0, eval_data,
          eval_datalen);
    
    test(network, test_data, datalen);
    
    deleteNetwork(network);
    free(train_data);
    free(test_data);
    
    network = createNetwork();
    network->flags |= FLAG_ONEHOT;
    addLayer(network, FullyConnected, RNN_INPUT_SIZE, NULL);
    addLayer(network, Recurrent, RNN_HIDDEN_SIZE, NULL);
    addLayer(network, SoftMax, RNN_INPUT_SIZE, NULL);
    network->layers[network->size - 1]->flags |= FLAG_ONEHOT;
    
    train(network, rnn_train_data, 10, EPOCHS, RNN_LEARNING_RATE, 1, 0,
          NULL, 0);
    
    deleteNetwork(network);
    
    return 0;
}
