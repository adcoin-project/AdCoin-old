// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "uritests.h"

#include "guiutil.h"
#include "walletmodel.h"
#include <iostream>
#include <QUrl>

void URITests::uriTests()
{
    SendCoinsRecipient rv;
    QUrl uri;
    uri.setUrl(QString("adcoin:AXs92pSFd8nss5qhg2rz3oANBe97tLRfWV?req-dontexist="));
    QVERIFY(!GUIUtil::parseBitcoinURI(uri, &rv));

    uri.setUrl(QString("adcoin:AXs92pSFd8nss5qhg2rz3oANBe97tLRfWV?dontexist="));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("AXs92pSFd8nss5qhg2rz3oANBe97tLRfWV"));
    QVERIFY(rv.label == QString());
    QVERIFY(rv.amount == 0);

    uri.setUrl(QString("adcoin:AXs92pSFd8nss5qhg2rz3oANBe97tLRfWV?label=Wikipedia Example Address"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("AXs92pSFd8nss5qhg2rz3oANBe97tLRfWV"));
    QVERIFY(rv.label == QString("Wikipedia Example Address"));
    QVERIFY(rv.amount == 0);

    uri.setUrl(QString("adcoin:AXs92pSFd8nss5qhg2rz3oANBe97tLRfWV?amount=0.001"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("AXs92pSFd8nss5qhg2rz3oANBe97tLRfWV"));
    QVERIFY(rv.label == QString());
    QVERIFY(rv.amount == 100000);

    uri.setUrl(QString("adcoin:AXs92pSFd8nss5qhg2rz3oANBe97tLRfWV?amount=1.001"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("AXs92pSFd8nss5qhg2rz3oANBe97tLRfWV"));
    QVERIFY(rv.label == QString());
    QVERIFY(rv.amount == 100100000);

    uri.setUrl(QString("adcoin:AXs92pSFd8nss5qhg2rz3oANBe97tLRfWV?amount=100&label=Wikipedia Example"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("AXs92pSFd8nss5qhg2rz3oANBe97tLRfWV"));
    QVERIFY(rv.amount == 10000000000LL);
    QVERIFY(rv.label == QString("Wikipedia Example"));

    uri.setUrl(QString("adcoin:AXs92pSFd8nss5qhg2rz3oANBe97tLRfWV?message=Wikipedia Example Address"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("AXs92pSFd8nss5qhg2rz3oANBe97tLRfWV"));
    QVERIFY(rv.label == QString());

    QVERIFY(GUIUtil::parseBitcoinURI("adcoin://AXs92pSFd8nss5qhg2rz3oANBe97tLRfWV?message=Wikipedia Example Address", &rv));
    std::cout << "address: " << rv.address.toUtf8().constData() << std::endl;

    QVERIFY(rv.address == QString("AXs92pSFd8nss5qhg2rz3oANBe97tLRfWV"));
    QVERIFY(rv.label == QString());

    uri.setUrl(QString("adcoin:AXs92pSFd8nss5qhg2rz3oANBe97tLRfWV?req-message=Wikipedia Example Address"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));

    uri.setUrl(QString("adcoin:AXs92pSFd8nss5qhg2rz3oANBe97tLRfWV?amount=1,000&label=Wikipedia Example"));
    QVERIFY(!GUIUtil::parseBitcoinURI(uri, &rv));

    uri.setUrl(QString("adcoin:AXs92pSFd8nss5qhg2rz3oANBe97tLRfWV?amount=1,000.0&label=Wikipedia Example"));
    QVERIFY(!GUIUtil::parseBitcoinURI(uri, &rv));
}
