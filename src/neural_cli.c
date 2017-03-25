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
#include <string.h>
#include <stdlib.h>
#include "neural.h"
#include "mnist.h"

#define PROGRAM_NAME        "CNeural CLI"

#define CONV_FEATURE_COUNT  20
#define CONV_REGION_SIZE    5
#define POOL_REGION_SIZE    2

#define MAX_FILENAME_LEN    255
#define EPOCHS              30
#define LEARNING_RATE       1.5
#define BATCH_SIZE          10

static PSLayerType getLayerType(char * name, PSNeuralNetwork * network) {
    if (strcmp("fully_connected", name) == 0)
        return FullyConnected;
    else if (strcmp("convolutional", name) == 0)
        return Convolutional;
    else if (strcmp("pooling", name) == 0)
        return Pooling;
    else if (strcmp("softmax", name) == 0)
        return SoftMax;
    else if (strcmp("recurrent", name) == 0)
        return Recurrent;
    else if (strcmp("lstm", name) == 0)
        return LSTM;
    else {
        fprintf(stderr, "Unkown layer type %s\n", name);
        PSDeleteNetwork(network);
        exit(1);
    }
}

static void getTempFileName(const char * prefix, char * buffer) {
    FILE * urand = fopen("/dev/urandom", "r");
    char buff[4];
    fgets(buff, 4, urand);
    sprintf(buffer, "/tmp/%s-%02x%02x%02x%02x.data",
            prefix,
            (unsigned char) buff[0],
            (unsigned char) buff[1],
            (unsigned char) buff[2],
            (unsigned char) buff[3]);
    fclose(urand);
}

double * training_data = NULL;
double * test_data = NULL;
double * validation_data = NULL;
int testlen = 0;
int datalen = 0;
int valdlen = 0;
int train_dataset_len = 0;
int eval_dataset_len = 0;
int epochs = EPOCHS;
float learning_rate = LEARNING_RATE;
int batch_size = BATCH_SIZE;
char outputFile[255];

void print_help(const char* program_path);

int main(int argc, char ** argv) {
    PSNeuralNetwork * network = PSCreateNetwork("CLI Network");
    int i, j;
    outputFile[0] = 0;
    int training_flags = 0;
    for (i = 1; i < argc; i++) {
        //printf("ARG[%d]: %s\n", i, argv[i]);
        char * arg = argv[i];
        if (strcmp("--load", arg) == 0 && ++i < argc) {
            char * file = argv[i];
            int loaded = PSLoadNetwork(network, file);
            if (!loaded) {
                PSDeleteNetwork(network);
                fprintf(stderr, "Could not load pretrained network %s\n", file);
                exit(1);
            }
            continue;
        }
        
        if (strcmp("--save", arg) == 0 && ++i < argc) {
            char * file = argv[i];
            if (strlen(file) > 254) {
                fprintf(stderr, "--save filename length must be <= 254");
            } else {
                sprintf(outputFile, "%s", file);
            }
            continue;
        }
        
        if (strcmp("--onehot", arg) == 0) {
            if (network->size == 0) network->flags |= FLAG_ONEHOT;
            else network->layers[network->size - 1]->flags |= FLAG_ONEHOT;
            continue;
        }
        
        if (strcmp("--layer", arg) == 0 && ++i < argc) {
            char * type = argv[i];
            PSLayerType ltype = getLayerType(type, network);
            if ((i + 1) >= argc) {
                break;
            }
            if (Convolutional == ltype) {
                PSLayerParameters * params = NULL;
                params = PSCreateConvolutionalParameters(CONV_FEATURE_COUNT,
                                                         CONV_REGION_SIZE,
                                                         1, 0, 0);
                double * lparams = params->parameters;
                for (j = i + 1; j < argc; j++) {
                    char * carg = argv[j];
                    if (strcmp("--feature-count", carg) == 0 && ++j < argc) {
                        int fcount = 0;
                        char * fcstr = argv[j];
                        int matched = sscanf(fcstr, "%d", &fcount);
                        if (!matched) {
                            fprintf(stderr, "Invalid feature count %s\n",fcstr);
                            continue;
                        }
                        i = j - 1;
                        lparams[PARAM_FEATURE_COUNT] = (double) fcount;
                    } else if (strcmp("--region-size", carg) == 0 && ++j<argc) {
                        int rsize = 0;
                        char * rsstr = argv[j];
                        int matched = sscanf(rsstr, "%d", &rsize);
                        if (!matched) {
                            fprintf(stderr, "Invalid region size %s\n", rsstr);
                            continue;
                        }
                        i = j - 1;
                        lparams[PARAM_REGION_SIZE] = (double) rsize;
                    } else if (strcmp("--stride", carg) == 0 && ++j < argc) {
                        int stride = 0;
                        char * ststr = argv[j];
                        int matched = sscanf(ststr, "%d", &stride);
                        if (!matched) {
                            fprintf(stderr, "Invalid stride %s\n", ststr);
                            continue;
                        }
                        i = j - 1;
                        lparams[PARAM_STRIDE] = (double) stride;
                    } else if (strcmp("--use-relu", carg) == 0) {
                        i = j - 1;
                        lparams[PARAM_USE_RELU] = 1.0;
                    } else {
                        break;
                    }
                }
                PSAddConvolutionalLayer(network, params);
            } else if (Pooling == ltype) {
                PSLayerParameters * params = NULL;
                params = PSCreateConvolutionalParameters(0, POOL_REGION_SIZE,
                                                         POOL_REGION_SIZE,
                                                         0, 0);
                double * lparams = params->parameters;
                for (j = i + 1; j < argc; j++) {
                    char * carg = argv[j];
                    if (strcmp("--region-size", carg) == 0 && ++j < argc) {
                        int rsize = 0;
                        char * rsstr = argv[j];
                        int matched = sscanf(rsstr, "%d", &rsize);
                        if (!matched) {
                            fprintf(stderr, "Invalid region size %s\n", rsstr);
                            continue;
                        }
                        lparams[PARAM_REGION_SIZE] = (double) rsize;
                    } else {
                        break;
                    }
                }
                PSAddPoolingLayer(network, params);
            } else {
                int size = 0;
                char * sizestr = argv[++i];
                int matched = sscanf(sizestr, "%d", &size);
                if (!matched) {
                    fprintf(stderr, "Invalid size %s\n", sizestr);
                    PSDeleteNetwork(network);
                    exit(1);
                }
                PSAddLayer(network, ltype, size, NULL);
            }
            continue;
        }
        
        if (strcmp("--train", arg) == 0 && ++i < argc) {
            int mnist = 0;
            if (strcmp("--mnist", argv[i]) == 0) {
                mnist = 1;
            }
            if (!mnist) {
                fprintf(stderr, "Only MNIST data supported for train ATM :(\n");
                PSDeleteNetwork(network);
                exit(1);
            } else {
                train_dataset_len = 50000;
                eval_dataset_len = 10000;
            }
            if ((i + 2) < argc) {
                char * imgfile = argv[++i];
                char * lblfile = argv[++i];
                datalen = loadMNISTData(TRAINING_DATA, imgfile, lblfile,
                                        &training_data);
                if (datalen == 0 || training_data == NULL) {
                    fprintf(stderr, "Could not load training data!\n");
                    PSDeleteNetwork(network);
                    exit(1);
                }
            } else {
                fprintf(stderr, "Missing MNIST training data files\n");
                PSDeleteNetwork(network);
                exit(1);
            }
            continue;
        }
        
        if (strcmp("--test", arg) == 0 && ++i < argc) {
            int mnist = 0;
            if (strcmp("--mnist", argv[i])) {
                mnist = 1;
            }
            if (!mnist) {
                fprintf(stderr, "Only MNIST data supported ATM :(\n");
                PSDeleteNetwork(network);
                exit(1);
            }
            if ((i + 2) < argc) {
                char * imgfile = argv[++i];
                char * lblfile = argv[++i];
                testlen = loadMNISTData(TEST_DATA, imgfile, lblfile,
                                        &test_data);
                if (testlen == 0 || test_data == NULL) {
                    fprintf(stderr, "Could not load test data!\n");
                    PSDeleteNetwork(network);
                    exit(1);
                }
            } else {
                fprintf(stderr, "Missing MNIST test data files\n");
                PSDeleteNetwork(network);
                exit(1);
            }
            continue;
        }
        
        if (strcmp("--training-datalen", arg) == 0 && ++i < argc) {
            char * len_s = argv[i];
            int matched = sscanf(len_s, "%d", &train_dataset_len);
            if (!matched)
                fprintf(stderr, "Invalid train. data len. %s\n", len_s);
            continue;
        }
        
        if (strcmp("--validation-datalen", arg) == 0 && ++i < argc) {
            char * len_s = argv[i];
            int matched = sscanf(len_s, "%d", &eval_dataset_len);
            if (!matched)
                fprintf(stderr, "Invalid valid. data len. %s\n", len_s);
            continue;
        }
        
        if (strcmp("--epochs", arg) == 0 && ++i < argc) {
            char * len_s = argv[i];
            int matched = sscanf(len_s, "%d", &epochs);
            if (!matched)
                fprintf(stderr, "Invalid epochs %s\n", len_s);
            continue;
        }
        
        if (strcmp("--batch-size", arg) == 0 && ++i < argc) {
            char * len_s = argv[i];
            int matched = sscanf(len_s, "%d", &batch_size);
            if (!matched)
                fprintf(stderr, "Invalid batch size %s\n", len_s);
            continue;
        }
        
        if (strcmp("--learning-rate", arg) == 0 && ++i < argc) {
            char * len_s = argv[i];
            int matched = sscanf(len_s, "%f", &learning_rate);
            if (!matched)
                fprintf(stderr, "Invalid learning rate %s\n", len_s);
            continue;
        }
        
        if (strcmp("--training-no-shuffle", arg) == 0) {
            training_flags |= TRAINING_NO_SHUFFLE;
            continue;
        }
        
        if (strcmp("--training-adjust-rate", arg) == 0) {
            training_flags |= TRAINING_ADJUST_RATE;
            continue;
        }
        
        if (strcmp("-v", arg) == 0 || strcmp("--version", arg) == 0) {
            printf("%s v%s\n", PROGRAM_NAME, NN_VERSION);
            exit(0);
        }
        
        if (strcmp("-h", arg) == 0 || strcmp("--help", arg) == 0) {
            print_help(argv[0]);
            exit(0);
        }
        
    }
    if (training_data != NULL) {
        int element_size = network->input_size + network->output_size;
        int element_count = datalen / element_size;
        if (element_count < train_dataset_len) {
            fprintf(stderr, "Loaded dataset elements %d < %d\n", element_count,
                   train_dataset_len);
            PSDeleteNetwork(network);
            return 1;
        } else {
            int remaining = element_count - train_dataset_len;
            if (remaining < eval_dataset_len && eval_dataset_len > 0) {
                fprintf(stderr, "WARNING: eval. dataset cannot be > %d!\n",
                        remaining);
                eval_dataset_len = remaining;
            }
            if (remaining == 0) {
                fprintf(stderr,
                        "WARNING: no dataset remaining for evaluation!\n");
                eval_dataset_len = remaining;
            }
            datalen = train_dataset_len * element_size;
            if (eval_dataset_len == 0) validation_data = NULL;
            else {
                validation_data = training_data + datalen;
                valdlen = eval_dataset_len * element_size;
            }
        }
        PSTrain(network, training_data, datalen, epochs, learning_rate,
                batch_size, training_flags, validation_data, valdlen);
        free(training_data);
    }
    if (test_data != NULL) {
        PSTest(network, test_data, testlen);
        free(test_data);
    }
    if (!strlen(outputFile)) {
        getTempFileName("saved-network", outputFile);
    }
    int saved = PSSaveNetwork(network, outputFile);
    if (!saved) {
        fprintf(stderr, "Could not save network to %s\n", outputFile);
    } else {
        printf("Network saved to %s\n", outputFile);
    }
    PSDeleteNetwork(network);
    return 0;
}

void print_help(const char* program_path) {
    printf("Usage: %s [OPTIONS]\n\n", program_path);
    printf("OPTIONS:\n");
    printf("        --load PRETRAINED           Load a pretrained network\n");
    printf("        --save FILE                 Save network\n");
    printf("        --layer TYPE SIZE|OPTIONS   Add layer\n");
    printf("        --onehot                    "
           "Sets one-hot-vector flag for input\n");
    printf("                                    "
           "(if before 1st layer) or desired output\n");
    printf("                                    (if after output layer)\n");
    printf("        --train TRAIN DATASET       Train network\n");
    printf("        --test TEST DATASET         Perform tests\n");
    printf("        --training-datalen LEN      Training data length\n");
    printf("        --validation-datalen LEN    Validation data length\n");
    printf("        --epochs EPOCHS             Training epochs (def. %d)\n",
           EPOCHS);
    printf("        --batch-size SIZE           Train. batch size (def. %d)\n",
           BATCH_SIZE);
    printf("        --learning-rate SIZE        Train. learn rate (def. %f)\n",
           LEARNING_RATE);
    printf("        --training-no-shuffle       Prevent dataset shuffle\n");
    printf("        --training-adjust-rate      Auto-adjust learn rate\n");
    printf("    -v, --version                   Print version\n");
    printf("    -h, --help                      Print this help\n");
    printf("\n");
    printf("LAYER TYPES:\n");
    int i;
    for (i = 0; i < LAYER_TYPES; i++) {
        PSLayerType type = (PSLayerType) i;
        printf("    %s\n", getLabelForType(type));
    }
    printf("\n");
    printf("LAYER OPTIONS:\n");
    printf("        --feature-count COUNT     Convolutional features"
           " (def. %d)\n", CONV_FEATURE_COUNT);
    printf("        --region-size SIZE        Convolutional region size"
           " (def. %d)\n", CONV_REGION_SIZE);
    printf("        --stride STRIDE           Convolutional region stride"
           " (def. 1)\n");
    printf("        --use-relu                Use ReLU activation (for "
           "Convolutional Layers)\n");
    printf("\n");
}
