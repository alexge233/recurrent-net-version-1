#include "result_predict.h"

using namespace cv;
using namespace std;

Mat 
resultPredict(std::vector<Mat> &x, std::vector<Hl> &hLayers, Smr &smr){

    int T = x.size();
    // hidden layer forward
    std::vector<std::vector<Mat> > acti;
    std::vector<Mat> tmp_vec;
    acti.push_back(tmp_vec);
    for(int i = 0; i < T; ++i){
        acti[0].push_back(x[i]);
    }
    for(int i = 1; i <= hiddenConfig.size(); ++i){
        // for each hidden layer
        acti.push_back(tmp_vec);
        for(int j = 0; j < T; ++j){
            // for each time slot
            Mat tmpacti = hLayers[i - 1].U * acti[i - 1][j];
            if(j > 0) tmpacti += hLayers[i - 1].W * acti[i][j - 1];
            tmpacti = ReLU(tmpacti);
            if(hiddenConfig[i - 1].DropoutRate < 1.0) tmpacti = tmpacti.mul(hiddenConfig[i - 1].DropoutRate);
            acti[i].push_back(tmpacti);
        }
    }
    // softmax layer forward
    Mat M = smr.W * acti[acti.size() - 1][T - 1];
    Mat result = Mat::zeros(1, M.cols, CV_64FC1);

    double minValue, maxValue;
    Point minLoc, maxLoc;
    for(int i = 0; i < M.cols; i++){
        minMaxLoc(M(Rect(i, 0, 1, M.rows)), &minValue, &maxValue, &minLoc, &maxLoc);
        result.ATD(0, i) = (int)maxLoc.y;
    }
    acti.clear();
    std::vector<std::vector<Mat> >().swap(acti);
    return result;
}

void 
testNetwork(const std::vector<std::vector<int> > &x, std::vector<std::vector<int> > &y, std::vector<Hl> &HiddenLayers, Smr &smr, 
             std::vector<string> &re_wordmap){

    // Test use test set
    // Because it may leads to lack of memory if testing the whole dataset at 
    // one time, so separate the dataset into small pieces of batches (say, batch size = 20).
    // 
    int batchSize = 50;
    Mat result = Mat::zeros(1, x.size(), CV_64FC1);

    std::vector<std::vector<int> > tmpBatch;
    int batch_amount = x.size() / batchSize;
    for(int i = 0; i < batch_amount; i++){
        for(int j = 0; j < batchSize; j++){
            tmpBatch.push_back(x[i * batchSize + j]);
        }
        std::vector<Mat> sampleX;
        getDataMat(tmpBatch, sampleX, re_wordmap);
        Mat resultBatch = resultPredict(sampleX, HiddenLayers, smr);
        Rect roi = Rect(i * batchSize, 0, batchSize, 1);
        resultBatch.copyTo(result(roi));
        tmpBatch.clear();
        sampleX.clear();
    }
    if(x.size() % batchSize){
        for(int j = 0; j < x.size() % batchSize; j++){
            tmpBatch.push_back(x[batch_amount * batchSize + j]);
        }
        std::vector<Mat> sampleX;
        getDataMat(tmpBatch, sampleX, re_wordmap);
        Mat resultBatch = resultPredict(sampleX, HiddenLayers, smr);
        Rect roi = Rect(batch_amount * batchSize, 0, x.size() % batchSize, 1);
        resultBatch.copyTo(result(roi));
        ++ batch_amount;
        tmpBatch.clear();
        sampleX.clear();
    }
    Mat sampleY = Mat::zeros(1, y.size(), CV_64FC1);
    getLabelMat(y, sampleY);

    Mat err;
    sampleY.copyTo(err);
    err -= result;
    int correct = err.cols;
    for(int i=0; i<err.cols; i++){
        if(err.ATD(0, i) != 0) --correct;
    }
    cout<<"######################################"<<endl;
    cout<<"## test result. "<<correct<<" correct of "<<err.cols<<" total."<<endl;
    cout<<"## Accuracy is "<<(double)correct / (double)(err.cols)<<endl;
    cout<<"######################################"<<endl;
    result.release();
    err.release();
}
