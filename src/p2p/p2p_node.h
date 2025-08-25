// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#pragma once

#include <functional>
#include <list>

#include <logging/logger_ref.h>
#include <syst/context_group.h>
#include <syst/dispatcher.h>
#include <syst/event.h>
#include <syst/tcp_listener.h>
#include <syst/timer.h>

#include "ip2p_node_internal.h"
#include "istream_serializable.h"
#include "net_node_config.h"

#include <p2p/p2p_interfaces.h>
#include <p2p/p2p_node_config.h>
#include <p2p/p2p_protocol_definitions.h>
#include <p2p/peer_list_manager.h>
#include <p2p/peerlist.h>

namespace mevacoin
{

    class P2pContext;
    class P2pConnectionProxy;

    class P2pNode : public IP2pNode,
                    public IStreamSerializable,
                    IP2pNodeInternal
    {

    public:
        P2pNode(
            const P2pNodeConfig &cfg,
            syst::Dispatcher &dispatcher,
            std::shared_ptr<logging::ILogger> log,
            const crypto::Hash &genesisHash,
            uint64_t peerId);

        ~P2pNode();

        // IP2pNode
        virtual void stop() override;

        // IStreamSerializable
        virtual void save(std::ostream &os) override;
        virtual void load(std::istream &in) override;

        // P2pNode
        void start();
        void serialize(ISerializer &s);

    private:
        typedef std::unique_ptr<P2pContext> ContextPtr;
        typedef std::list<ContextPtr> ContextList;

        logging::LoggerRef logger;
        bool m_stopRequested;
        const P2pNodeConfig m_cfg;
        const uint64_t m_myPeerId;
        const CORE_SYNC_DATA m_genesisPayload;

        syst::Dispatcher &m_dispatcher;
        syst::ContextGroup workingContextGroup;
        syst::TcpListener m_listener;
        syst::Timer m_connectorTimer;
        PeerlistManager m_peerlist;
        ContextList m_contexts;
        syst::Event m_queueEvent;
        std::deque<std::unique_ptr<IP2pConnection>> m_connectionQueue;

        // IP2pNodeInternal
        virtual const CORE_SYNC_DATA &getGenesisPayload() const override;
        virtual std::list<PeerlistEntry> getLocalPeerList() override;
        virtual basic_node_data getNodeData() const override;
        virtual uint64_t getPeerId() const override;

        virtual void handleNodeData(const basic_node_data &node, P2pContext &ctx) override;
        virtual bool handleRemotePeerList(const std::list<PeerlistEntry> &peerlist, time_t local_time) override;
        virtual void tryPing(P2pContext &ctx) override;

        // spawns
        void acceptLoop();
        void connectorLoop();

        // connection related
        void connectPeers();
        void connectPeerList(const std::vector<NetworkAddress> &peers);
        bool isPeerConnected(const NetworkAddress &address);
        bool isPeerUsed(const PeerlistEntry &peer);
        ContextPtr tryToConnectPeer(const NetworkAddress &address);
        bool fetchPeerList(ContextPtr connection);

        // making and processing connections
        size_t getOutgoingConnectionsCount() const;
        void makeExpectedConnectionsCount(const Peerlist &peerlist, size_t connectionsCount);
        bool makeNewConnectionFromPeerlist(const Peerlist &peerlist);
        void preprocessIncomingConnection(ContextPtr ctx);
        void enqueueConnection(std::unique_ptr<P2pConnectionProxy> proxy);
        std::unique_ptr<P2pConnectionProxy> createProxy(ContextPtr ctx);
    };

}
