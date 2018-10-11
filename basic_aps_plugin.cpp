/*
 * Copyright (C) 2014 dresden elektronik ingenieurtechnik gmbh.
 * All rights reserved.
 *
 * The software in this package is published under the terms of the BSD
 * style license a copy of which has been included with this distribution in
 * the LICENSE.txt file.
 *
 */

#include <QtPlugin>
#include <QTimer>
#include "basic_aps_plugin.h"

/*! Duration for which the idle state will be hold before state machine proceeds. */
#define IDLE_TIMEOUT                  (10 * 1000)

/*! Duration to wait for Match_Descr_rsp frames after sending the request. */
#define WAIT_MATCH_DESCR_RESP_TIMEOUT (10 * 1000)

/*! Plugin constructor.
    \param parent - the parent object
 */
BasicApsPlugin::BasicApsPlugin(QObject *parent) :
    QObject(parent)
{
    m_state = StateIdle;
    // keep a pointer to the ApsController
    m_apsCtrl = deCONZ::ApsController::instance();
    DBG_Assert(m_apsCtrl != 0);

    // APSDE-DATA.confirm handler
    connect(m_apsCtrl, SIGNAL(apsdeDataConfirm(const deCONZ::ApsDataConfirm&)),
            this, SLOT(apsdeDataConfirm(const deCONZ::ApsDataConfirm&)));

    // APSDE-DATA.indication handler
    connect(m_apsCtrl, SIGNAL(apsdeDataIndication(const deCONZ::ApsDataIndication&)),
            this, SLOT(apsdeDataIndication(const deCONZ::ApsDataIndication&)));

    // timer used for state changes and timeouts
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    connect(m_timer, SIGNAL(timeout()),
            this, SLOT(timerFired()));

    // start the state machine
    m_timer->start(1000);
}

/*! Deconstructor for plugin.
 */
BasicApsPlugin::~BasicApsPlugin()
{
    m_apsCtrl = 0;
}

/*! APSDE-DATA.indication callback.
    \param ind - the indication primitive
    \note Will be called from the main application for every incoming indication.
    Any filtering for nodes, profiles, clusters must be handled by this plugin.
 */
void BasicApsPlugin::apsdeDataIndication(const deCONZ::ApsDataIndication &ind)
{
    if (ind.profileId() == ZDP_PROFILE_ID)
    {
        if (ind.clusterId() == ZDP_MATCH_DESCRIPTOR_RSP_CLID)
        {
            handleMatchDescriptorResponse(ind);
        }
    }
}

/*! APSDE-DATA.confirm callback.
    \param conf - the confirm primitive
    \note Will be called from the main application for each incoming confirmation,
    even if the APSDE-DATA.request was not issued by this plugin.
 */
void BasicApsPlugin::apsdeDataConfirm(const deCONZ::ApsDataConfirm &conf)
{
    std::list<deCONZ::ApsDataRequest>::iterator i = m_apsReqQueue.begin();
    std::list<deCONZ::ApsDataRequest>::iterator end = m_apsReqQueue.end();

    // search the list of currently active requests
    // and check if the confirmation belongs to one of them
    for (; i != end; ++i)
    {
        if (i->id() == conf.id())
        {
            m_apsReqQueue.erase(i);

            if (conf.status() == deCONZ::ApsSuccessStatus)
            {
                stateMachineEventHandler(EventSendDone);
            }
            else
            {
                DBG_Printf(DBG_INFO, "APS-DATA.confirm failed with status: 0x%02X\n", conf.status());
                stateMachineEventHandler(EventSendFailed);
            }
            return;
        }
    }
}

/*! Handles a match descriptor response.
    \param ind a ZDP Match_Descr_rsp
 */
void BasicApsPlugin::handleMatchDescriptorResponse(const deCONZ::ApsDataIndication &ind)
{

    QDataStream stream(ind.asdu());
    stream.setByteOrder(QDataStream::LittleEndian);

    uint8_t zdpSeq;
    uint8_t status;
    uint16_t nwkAddrOfInterest;
    uint8_t matchLength;
    uint8_t endpoint;

    stream >> zdpSeq;

    // only handle the Match_Descr_rsp which belongs to our request
    if (zdpSeq != m_matchDescrZdpSeq)
    {
        return;
    }

    stream >> status;

    DBG_Printf(DBG_INFO, "received match descriptor response (id: %u) from %s\n", m_matchDescrZdpSeq, qPrintable(ind.srcAddress().toStringExt()));

    if (status == 0x00) // SUCCESS
    {
        stream >> nwkAddrOfInterest;
        stream >> matchLength;

        while (matchLength && !stream.atEnd())
        {
            matchLength--;
            stream >> endpoint;
            DBG_Printf(DBG_INFO, "\tmatch descriptor endpoint: 0x%02X\n", endpoint);
        }

        // done restart state machine
        if (m_state == StateWaitMatchDescriptorResponse)
        {
            setState(StateIdle);
            m_timer->stop();
            m_timer->start(IDLE_TIMEOUT);
        }
    }
}

/*! Handler for simple timeout timer.
 */
void BasicApsPlugin::timerFired()
{
    stateMachineEventHandler(EventTimeout);
}

/*! Sends a ZDP Match_Descr_req for On/Off cluster (ClusterID=0x0006).
    \return true if request was added to queue
 */
bool BasicApsPlugin::sendMatchDescriptorRequest()
{
    DBG_Assert(m_state == StateIdle);

    if (m_apsCtrl->networkState() != deCONZ::InNetwork)
    {
        return false;
    }

    deCONZ::ApsDataRequest apsReq;

    // set destination addressing
    apsReq.setDstAddressMode(deCONZ::ApsNwkAddress);
    apsReq.dstAddress().setNwk(deCONZ::BroadcastRxOnWhenIdle);
    apsReq.setDstEndpoint(ZDO_ENDPOINT);
    apsReq.setSrcEndpoint(ZDO_ENDPOINT);
    apsReq.setProfileId(ZDP_PROFILE_ID);
    apsReq.setClusterId(ZDP_MATCH_DESCRIPTOR_CLID);

    // prepare payload
    QDataStream stream(&apsReq.asdu(), QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);

    // generate and remember a new ZDP transaction sequence number
    m_matchDescrZdpSeq = (uint8_t)qrand();

    // write payload according to ZigBee specification (2.4.3.1.7 Match_Descr_req)
    // here we search for ZLL device which provides a OnOff server cluster
    // NOTE: explicit castings ensure correct size of the fields
    stream << m_matchDescrZdpSeq; // ZDP transaction sequence number
    stream << (quint16)deCONZ::BroadcastRxOnWhenIdle; // NWKAddrOfInterest
    stream << (quint16)ZLL_PROFILE_ID; // ProfileID
    stream << (quint8)0x01; // NumInClusters
    stream << (quint16)0x0006; // OnOff ClusterID
    stream << (quint8)0x00; // NumOutClusters

    if (m_apsCtrl && (m_apsCtrl->apsdeDataRequest(apsReq) == deCONZ::Success))
    {
        // remember request
        m_apsReqQueue.push_back(apsReq);
        return true;
    }

    return false;
}

/*! Sets the state machine state.
    \param state the new state
 */
void BasicApsPlugin::setState(BasicApsPlugin::State state)
{
    if (m_state != state)
    {
        m_state = state;
    }
}

/*! deCONZ will ask this plugin which features are supported.
    \param feature - feature to be checked
    \return true if supported
 */
bool BasicApsPlugin::hasFeature(Features feature)
{
    switch (feature)
    {
    default:
        break;
    }

    return false;
}

/*! Main state machine event handler.
    \param event the event which occured
 */
void BasicApsPlugin::stateMachineEventHandler(BasicApsPlugin::Event event)
{
    if (m_state == StateIdle)
    {
        if (event == EventTimeout)
        {
            m_apsReqQueue.clear();

            if (sendMatchDescriptorRequest())
            {
                DBG_Printf(DBG_INFO, "send match descriptor request (id: %u)\n", m_matchDescrZdpSeq);
                setState(StateWaitMatchDescriptorResponse);
                m_timer->start(WAIT_MATCH_DESCR_RESP_TIMEOUT);
            }
            else
            {
                // try again later
                m_timer->start(IDLE_TIMEOUT);
            }
        }
    }
    else if (m_state == StateWaitMatchDescriptorResponse)
    {
        if (event == EventSendDone)
        {
            DBG_Printf(DBG_INFO, "send match descriptor request done (id: %u)\n", m_matchDescrZdpSeq);
        }
        else if (event == EventSendFailed)
        {
            DBG_Printf(DBG_INFO, "send match descriptor request failed (id: %u)\n", m_matchDescrZdpSeq);
            // go back to idle state and wait some time
            setState(StateIdle);
            m_timer->start(IDLE_TIMEOUT);
        }
        else if (event == EventTimeout)
        {
            DBG_Printf(DBG_INFO, "stop wait for match descriptor response (id: %u)\n", m_matchDescrZdpSeq);
            // go back to idle state and wait some time
            setState(StateIdle);
            m_timer->start(IDLE_TIMEOUT);
        }
    }
}

/*! Returns the name of this plugin.
 */
const char *BasicApsPlugin::name()
{
    return "Basic APS Plugin";
}

#if QT_VERSION < 0x050000
    Q_EXPORT_PLUGIN2(basic_aps_plugin, BasicApsPlugin)
#endif

