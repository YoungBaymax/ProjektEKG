#include "appcontroller.h"
#include "Common/ecgdata.h"
#include "Common/ecgentry.h"
#include "ECG_BASELINE/src/butter.h"
#include "ECG_BASELINE/src/kalman.h"
#include "ECG_BASELINE/src/movAvg.h"
#include "ECG_BASELINE/src/sgolay.h"
#include "ST_INTERVAL/ecgstanalyzer.h"
#include "HRV1/HRV1MainModule.h"

#include <QThread>

AppController::AppController(QObject *parent) : QObject(parent)
{
    //this->InitializeDependencies();
}
/*
void AppController::InitializeDependencies()
{
    //Ecg_BaselineModule *ecg_BaselineModule = new Ecg_BaselineModule();
    //this->supervisor = new SupervisoryModule("ECG_BASELINE", ecg_BaselineModule);
}
*/
void AppController::BindView(AirEcgMain *view)
{
    this->connect(view, SIGNAL(loadEntity(QString,QString)), this, SLOT(loadData(QString,QString)));
    this->connect(view, SIGNAL(switchSignal(int)), this, SLOT(switchSignal(int)));
    this->connect(this, SIGNAL(patientData(EcgData*)), view, SLOT(receivePatientData(EcgData*)));
    this->connect(view, SIGNAL(switchEcgBaseline(int)), this, SLOT(switchEcgBaseline(int)));
    this->connect(view, SIGNAL(switchRPeaks(unsigned char)), this, SLOT(switchRPeaks(unsigned char)));
    this->connect(view, SIGNAL(switchWaves_p_onset(bool)), this, SLOT(switchWaves_p_onset( bool)));
    this->connect(view, SIGNAL(switchTWA(unsigned char)), this, SLOT(switchTWA(unsigned char)));
    this->connect(view, SIGNAL(run()), this, SLOT(run()));
    this->connect(this, SIGNAL(processingResults(EcgData*)), view, SLOT(receiveResults(EcgData*)));
    this->connect(view, SIGNAL(qrsClassChanged(int,int)),this,SLOT(sendQRSData(int,int)));
    this->connect(this, SIGNAL(sendQRSData(QRSClass,int)),view,SLOT(receiveQRSData(QRSClass,int)));
    this->connect(view, SIGNAL(runSingle(QString)), this, SLOT(runSingle(QString)));

    this->connect(view, SIGNAL(runEcgBaseline()),this, SLOT (runEcgBaseline()));//example
    this->connect(view, SIGNAL(runAtrialFibr()) ,this, SLOT (runAtrialFibr()));
    this->connect(view, SIGNAL(runStInterval()) ,this, SLOT(runStInterval()));
    this->connect(view, SIGNAL(runHRV1())       ,this, SLOT (runHRV1()));

    this->connect(this, SIGNAL(EcgBaseline_done(EcgData*)),view, SLOT(drawEcgBaseline(EcgData*)));//example
    this->connect(this, SIGNAL( AtrialFibr_done(EcgData*)),view, SLOT(drawAtrialFibr(EcgData*)));
    this->connect(this, SIGNAL(StInterval_done(EcgData*)) ,view, SLOT(drawStInterval(EcgData*)));
    this->connect(this, SIGNAL(HRV1_done(EcgData*))       ,view, SLOT(drawHrv1(EcgData*)))      ;


    this->connect(this, SIGNAL(singleProcessingResult(bool, EcgData*)), view, SLOT(receiveSingleProcessingResult(bool, EcgData*)));
    this->connect(view, SIGNAL(qrsClustererChanged(ClustererType)),this,SLOT(qrsClustererChanged(ClustererType)));
    this->connect(view, SIGNAL(qrsGMaxClustersChanged(int)),this,SLOT(qrsGMaxClustersChanged(int)));
    this->connect(view, SIGNAL(qrsGMaxKIterations(int)),this,SLOT(qrsGMaxKIterations(int)));
    this->connect(view, SIGNAL(qrsGMinClustersChanged(int)),this,SLOT(qrsGMinClustersChanged(int)));
    this->connect(view, SIGNAL(qrsKClustersNumberChanged(int)),this,SLOT(qrsKClustersNumberChanged(int)));
    this->connect(view, SIGNAL(qrsMaxIterationsChanged(int)),this,SLOT(qrsMaxIterationsChanged(int)));
    this->connect(view, SIGNAL(qrsParallelExecutionChanged(bool)),this,SLOT(qrsParallelExecutionChanged(bool)));

}

void AppController::loadData(const QString &directory, const QString &name)
{
    EcgEntry *entry = new EcgEntry();
    QString *response = new QString("");
    if(!entry->Open(directory, name, *response))
    {
        return;
    }
    //if(this->entity)
        //this->supervisor->ResetModules();
    this->entity = entry->entity;

    emit patientData(this->entity);
}

void AppController::switchEcgBaseline(int type)
{
    if(this->entity)
        this->entity->settings->EcgBaselineMode = type;
}

void AppController::switchRPeaks(unsigned char type)
{
    if(this->entity)
        this->entity->settings->RPeaksMode = type;
}

void AppController::switchWaves_p_onset(bool check)
{
    //if(this->entity)
      //  this->entity->settings->
}

void AppController::sendQRSData(int index, int type)
{
    if (!this->entity || !this->entity->classes || !this->entity->waves || !this->entity->ecg_baselined || index < 0)
        return;
    QRSClass qrsSegment;

    if (type == 1)
    {
        if (this->entity->classes->count() > index)
        {
            qrsSegment = this->entity->classes->at(index);
        }
        else
            return;
    }
    else
    {
        if (this->entity->waves->count() <= index)
            return;
        int begin = this->entity->waves->at(index)->QRS_onset;
        int end = this->entity->waves->at(index)->QRS_end + 1;

        qrsSegment.representative = new QVector<double>();
        for(int i = begin; i < end; i++)
            qrsSegment.representative->append(this->entity->ecg_baselined->at(i));
    }
    emit sendQRSData(qrsSegment,type);
}

void AppController::switchTWA(unsigned char type)
{
    if(this->entity)
        this->entity->TWA_mode = type;
}

void AppController::qrsClustererChanged(ClustererType type)
{
    if(this->entity)
        this->entity->settings->qrsClassSettings.clusterer = type;
}

void AppController::qrsMaxIterationsChanged(int maxIters)
{
    if(this->entity)
        this->entity->settings->qrsClassSettings.maxIterations = maxIters;
}

void AppController::qrsParallelExecutionChanged(bool flag)
{
    if(this->entity)
        this->entity->settings->qrsClassSettings.parallelExecution = flag;
}

void AppController::qrsGMinClustersChanged(int minClusters)
{
    if(this->entity)
        this->entity->settings->qrsClassSettings.minClusterNo = minClusters;
}

void AppController::qrsGMaxClustersChanged(int maxClusters)
{
    if(this->entity)
        this->entity->settings->qrsClassSettings.maxClusterNo = maxClusters;
}

void AppController::qrsGMaxKIterations(int maxIters)
{
    if(this->entity)
        this->entity->settings->qrsClassSettings.insideIterations = maxIters;
}

void AppController::qrsKClustersNumberChanged(int noClusters)
{
    if(this->entity)
    {
        this->entity->settings->qrsClassSettings.minClusterNo = noClusters;
        this->entity->settings->qrsClassSettings.maxClusterNo = noClusters;
    }
}

void AppController::run()
{
    /*
    if(this->entity)
    {
        this->supervisor->entity = this->entity;
        connect(this->supervisor, SIGNAL(Finished()), this, SLOT(onThreadFinished()));
        this->supervisor->start();
    }*/
}

void AppController::onThreadFinished()
{
    emit this->processingResults(this->entity);
}

void AppController::runSingle(QString hash)
{



    /*
    if(this->entity)
    {
        bool processed = this->supervisor->RunSingle(this->entity, hash);
        emit singleProcessingResult(processed, this->entity);
    }
    */
}

void AppController::switchSignal(int index)
{

    this->entity->settings->signalIndex = index;
    this->ResetModules();
}
void AppController::runEcgBaseline()
{
    QLOG_INFO() <<"Ecg baseline started.";

    //QVector<double> test;
    //test << 0.5 << 0.5 << 0.5;
    KalmanFilter kalman;
    if (this->entity->ecg_baselined)
        this->entity->ecg_baselined->~QVector<double>();

    switch (this->entity->settings->EcgBaselineMode)
    {
    case 0: //butterworth
    case 1:
        QLOG_INFO() << "BASELINE/ Using moving average filter.";
        this->entity->ecg_baselined = new QVector<double>(processMovAvg(*(this->entity->GetCurrentSignal()),3));
        break;
    case 2: //savitzky-golay
        QLOG_INFO() << "BASELINE/ Using Savitzky-Golay filter.";
        this->entity->ecg_baselined = new QVector<double>(processSGolay(*(this->entity->GetCurrentSignal())));
        break;
    case 3:  //kalman
        QLOG_INFO() << "BASELINE/ Using kalman filter.";
        this->entity->ecg_baselined = new QVector<double>(kalman.processKalman(*(this->entity->GetCurrentSignal())));
        break;
    default:
        QLOG_INFO() << "BASELINE/ Using default filter.";
        this->entity->ecg_baselined = new QVector<double>(processMovAvg(*(this->entity->GetCurrentSignal()),3));
        break;
    }            

    QLOG_INFO() << "Ecg baseline done.";
    emit EcgBaseline_done(this->entity);
}

void AppController::runHRV1()
{
    QLOG_INFO() << "HRV1 started.";
    QVector<int> *wektor = new QVector<int>;
    int i=0;
    while (i<this->entity->Rpeaks->size())
    {
        wektor->append(this->entity->Rpeaks->at(i) - this->entity->Rpeaks->first());
        i++;
    }
    HRV1MainModule obiekt;
    obiekt.prepare(wektor,(int)this->entity->info->frequencyValue);
    HRV1BundleStatistical results = obiekt.evaluateStatistical();
    this->entity->Mean = results.RRMean;
    this->entity->SDNN = results.SDNN;
    this->entity->RMSSD= results.RMSSD;
    this->entity->RR50 = results.NN50;    //czy to jest to??
    this->entity->RR50Ratio=results.pNN50;//czy to jest to??
    this->entity->SDANN= results.SDANN;
    this->entity->SDANNindex=results.SDANNindex;
    this->entity->SDSD = results.SDSD;
    QLOG_INFO() << "HRV1 statistical done.";

    emit this->HRV1_done(this->entity);
}

void AppController::deep_copy_list(QList<int> *dest, QList<int> *src)
{
    //QList<int>::Iterator iter_dest = dest;
    QList<int>::Iterator iter_src  = src->begin();

    while (iter_src != src->end())
    {
        dest->append(*iter_src);
        iter_src++;
    }

}

void AppController::ResetModules()
{
    if (this->entity->ecg_baselined)
        this->entity->ecg_baselined->~QVector<double>();
    if (this->entity->PWaveStart)
        this->entity->PWaveStart->~QVector<QVector<double>::const_iterator>();
    if (this->entity->Rpeaks)
        this->entity->Rpeaks->~QVector<QVector<double>::const_iterator>();
}

void AppController::runAtrialFibr()
{
    QLOG_INFO() << "Start AtrialFibr";

    //wrzucenie przykladowych danych
    this->entity->Rpeaks     = new QVector<QVector<double>::const_iterator>
                              ({this->entity->ecg_baselined->begin() + 20,
                                this->entity->ecg_baselined->begin() + 40,
                                this->entity->ecg_baselined->begin() + 60,
                                this->entity->ecg_baselined->begin() + 80});

    this->entity->PWaveStart = new QVector<QVector<double>::const_iterator>
                              ({this->entity->ecg_baselined->begin() + 10,
                                this->entity->ecg_baselined->begin() + 70});

    AtrialFibrApi obiekt(*(this->entity->ecg_baselined),
                         *(this->entity->Rpeaks) ,
                         *(this->entity->PWaveStart) )   ;

    this->entity->PWaveOccurenceRatio= obiekt.GetPWaveOccurenceRatio();
    this->entity->RRIntDivergence    = obiekt.GetRRIntDivergence();
    this->entity->RRIntEntropy       = obiekt.GetRRIntEntropy();
    this->entity->AtrialFibr         = obiekt.isAtrialFibr();

    QLOG_INFO() << "Atrial_FIBR/ calculated parameters: \n"
                << "PWaveOccurenceRatio:" << QString::number(this->entity->PWaveOccurenceRatio) <<"\n";


    QLOG_INFO() << "AtrialFibr done";
    emit AtrialFibr_done(this->entity);//linia 37


}

void AppController::runStInterval()
{
    QLOG_INFO() << "Start StInterval";

    EcgStAnalyzer analyzer;
    analyzer.setAlgorithm(ST_LINEAR);
    analyzer.setDetectionSize(30);
    analyzer.setSmoothSize(4);
    analyzer.setMorphologyCoeff(6.0);
    analyzer.setLevelThreshold(0.15);
    analyzer.setSlopeThreshold(35);

//    QList<EcgStDescriptor> result;

    // teraz nalezy wywolac analyzer.analyze z odpowiednimi parametrami
    // result = analyzer.analyze(
    //  *this->entity->ecg_baselined, /* sygnal po baseline */
    //  *this->entity->Rpeaks, /* punkty Rpeak */
    //  ..., /* punkty J lub QRSend */
    //  ..., /* punkty Tend */
    //  ...  /* czestotliwosc probkowania sygnalu w Hz */
    // );
    //
    // operacja analizy zwraca liste deskryptorow interwalow ST,
    // ktora mozna zapisac w EcgData:
    // this->entity->STintervals = new QList<EcgDescriptor>(result);

//    emit StInterval_done(this->entity);
    QLOG_INFO() << "StInterval done";
}
