//=============================================================================================================
/**
* @file     unbiasedsquaredphaselagindex.cpp
* @author   Daniel Strohmeier <daniel.strohmeier@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     April, 2018
*
* @section  LICENSE
*
* Copyright (C) 2018, Daniel Strohmeier and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    UnbiasedSquaredPhaseLagIndex class definition.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "phaselagindex.h"
#include "unbiasedsquaredphaselagindex.h"
#include "network/networknode.h"
#include "network/networkedge.h"
#include "network/network.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QtConcurrent>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <unsupported/Eigen/FFT>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace CONNECTIVITYLIB;
using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

UnbiasedSquaredPhaseLagIndex::UnbiasedSquaredPhaseLagIndex()
{
}


//*******************************************************************************************************

Network UnbiasedSquaredPhaseLagIndex::unbiasedSquaredPhaseLagIndex(const QList<MatrixXd> &matDataList,
                                                                   const MatrixX3f& matVert,
                                                                   int iNfft, const QString &sWindowType)
{
    Network finalNetwork("Unbiased Squared Phase Lag Index");

    if(matDataList.empty()) {
        qDebug() << "UnbiasedSquaredPhaseLagIndex::unbiasedSquaredPhaseLagIndex - Input data is empty";
        return finalNetwork;
    }

    //Create nodes
    int rows = matDataList.first().rows();
    RowVectorXf rowVert = RowVectorXf::Zero(3);

    for(int i = 0; i < rows; ++i) {
        if(matVert.rows() != 0 && i < matVert.rows()) {
            rowVert(0) = matVert.row(i)(0);
            rowVert(1) = matVert.row(i)(1);
            rowVert(2) = matVert.row(i)(2);
        }

        finalNetwork.append(NetworkNode::SPtr(new NetworkNode(i, rowVert)));
    }

    //Calculate all-to-all coherence matrix over epochs
    QVector<MatrixXd> vecUnbiasedSquaredPLI = UnbiasedSquaredPhaseLagIndex::computeUnbiasedSquaredPLI(matDataList, iNfft, sWindowType);

    //Add edges to network
    for(int i = 0; i < vecUnbiasedSquaredPLI.length(); ++i) {
        for(int j = 0; j < matDataList.at(0).rows(); ++j) {
            MatrixXd matWeight = vecUnbiasedSquaredPLI.at(i).row(j).transpose();

            QSharedPointer<NetworkEdge> pEdge = QSharedPointer<NetworkEdge>(new NetworkEdge(finalNetwork.getNodes()[i], finalNetwork.getNodes()[j], matWeight));

            finalNetwork.getNodeAt(i)->append(pEdge);
            finalNetwork.append(pEdge);
        }
    }

    return finalNetwork;
}


//*************************************************************************************************************

QVector<MatrixXd> UnbiasedSquaredPhaseLagIndex::computeUnbiasedSquaredPLI(const QList<MatrixXd> &matDataList,
                                                                          int iNfft, const QString &sWindowType)
{
    int iNRows = matDataList.at(0).rows();
    int iNTrials = matDataList.length();

    // Compute standard PLI
    QVector<MatrixXd> vecUnbiasedSquaredPLI = PhaseLagIndex::computePLI(matDataList, iNfft, sWindowType);

    // Compute unbiased estimator according to Vinck et al., NeuroImage 55, pp. 1548-65, 2011
    for (int j = 0; j < iNRows; ++j) {
        vecUnbiasedSquaredPLI.replace(j, (double(iNTrials) * vecUnbiasedSquaredPLI.at(j).array().square() - 1.0) / double(iNTrials - 1));
    }
    return vecUnbiasedSquaredPLI;
}
