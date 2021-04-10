#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#ifndef MTL_PY_MTL_INTERFACE_H_
#define MTL_PY_MTL_INTERFACE_H_

#include "global/global.h"
#include <mockturtle/mockturtle.hpp>
#include <lorina/aiger.hpp>
// For balancing operations
#include <mockturtle/algorithms/balancing.hpp>
#include <mockturtle/algorithms/balancing/sop_balancing.hpp>

#include <bits/stdc++.h>

PROJECT_NAMESPACE_BEGIN

/// @class MTL_PY::MigStats
/// @brief stats of current design in MIG format
class MigStats
{
    public:
        explicit MigStats() = default;
        IndexType numIn() const { return _numIn; }
        IndexType numOut() const { return _numOut; }
        IndexType numLat() const { return _numLat; }
        IndexType numMigNodes() const { return _numNodes; }
        IndexType lev() const { return _lev; }

        void setNumIn(IndexType numIn) { _numIn = numIn; }
        void setNumOut(IndexType numOut) { _numOut = numOut; }
        void setNumLat(IndexType numLat) { _numLat = numLat; }
        void setNumMigNodes(IndexType numNodes) { _numNodes = numNodes; }
        void setLev(IndexType lev) { _lev = lev; }
    private:
        IndexType  _numIn = 0; ///< Input port
        IndexType  _numOut = 0; ///< Output port
        IndexType  _numLat = 0; ///< Number of latches
        IndexType  _numNodes = 0; ///< Number of AND
        IndexType  _lev = 0; ///< The deepest logic level
};


// object types
typedef enum { 
    MIG_NODE_CONST1 = 0,      //  0:  constant 1 node
    MIG_NODE_PI = 1,          //  1:  primary input terminal
    MIG_NODE_PO = 2,          //  2:  primary output terminal
    MIG_NODE_NONONO = 3,      //  3:  fanin 0: no inv fanin 1: no inv fanin 2: no inv
    MIG_NODE_INVNONO = 4,     //  4:  fanin 0: has inv fanin 1: no inv fanin 2: no inv
    MIG_NODE_INVINVNO = 5,    //  5:  fanin 0: has inv fanin 1: has inv fanin 2: no inv
    MIG_NODE_INVINVINV = 6,   //  6:  fanin 0: has inv fanin 1: has inv fanin 2: has inv   
    MIG_NODE_PIO = 7,         //  7:  Both PI and PO
    MIG_NODE_POC = 8,         //  8:  Both Constant and PO
    MIG_NODE_NUMBER = 9
} MigNodeType;

/// @class MTL_PY::MigNode
/// @brief Single MigNode of the graph. Basically a entry in adjacent list representation
class MigNode
{
    public:
        /// @brief default constructor
        explicit MigNode() = default;
        /// @brief whether has fanin 0
        /// @return if has fanin 0
        bool hasFanin0() { return _fanin0 != -1; }
        /// @brief get the index of fanin 0 node
        /// @return the index of fanin 0 node
        IntType fanin0() { AssertMsg(hasFanin0(), "The node does not has fanin 0!\n"); return _fanin0; }
        /// @brief whether has fanin 1
        /// @return if has fanin 1
        bool hasFanin1() { return _fanin1 != -1; }
        /// @brief get the index of fanin 1 node
        /// @return the index of fanin 1 node
        IntType fanin1() { AssertMsg(hasFanin1(), "The node does not has fanin 1!\n"); return _fanin1; }
        /// @brief whether has fanin 2
        /// @return if has fanin 2
        bool hasFanin2() { return _fanin2 != -1; }
        /// @brief get the index of fanin 2 node
        /// @return the index of fanin 2 node
        IntType fanin2() { AssertMsg(hasFanin2(), "The node does not has fanin 2!\n"); return _fanin2; }
        /// @brief Get number of fanouts
        /// @return number of fanouts
        IntType numFanouts() { return _fanout_size; }
        /// @brief Set the type of the node
        /// @param The type of the node. The type of defined in MigNodeType enum
        void setNodeType(IntType nodeType) { _nodeType = nodeType; }
        /// @brief Get the type of the node
        /// @param The type of the node.
        IntType nodeType()
        {
            AssertMsg(_nodeType != MIG_NODE_NUMBER, "Node type is unknown! \n");
            return _nodeType;
        }
        /// @brief Configure the node with Abc_Obj_t
        /// @param Pointer to Abc_Obj_t
        void configure(mockturtle::mig_network::signal a, mockturtle::mig_network::signal b, mockturtle::mig_network::signal c, IndexType num_fanouts, int type){
            _fanout_size = num_fanouts;
            if(type == 0){ // Normal Node
                _fanin0 = a.index;
                _fanin1 = b.index;
                _fanin2 = c.index;

                if(!a.complement && !b.complement && !c.complement){
                    this->setNodeType(MIG_NODE_NONONO);
                }
                else if(!a.complement && !b.complement && c.complement){
                    std::swap(_fanin0, _fanin2);
                    this->setNodeType(MIG_NODE_INVNONO);
                }
                else if(!a.complement && b.complement && !c.complement){
                    std::swap(_fanin0, _fanin1);
                    this->setNodeType(MIG_NODE_INVNONO);
                }
                else if(!a.complement && b.complement && c.complement){
                    std::swap(_fanin0, _fanin2);
                    this->setNodeType(MIG_NODE_INVINVNO);
                }
                else if(a.complement && !b.complement && !c.complement){
                    this->setNodeType(MIG_NODE_INVNONO);
                }
                else if(a.complement && !b.complement && c.complement){
                    std::swap(_fanin1, _fanin2);
                    this->setNodeType(MIG_NODE_INVINVNO);
                }
                else if(a.complement && b.complement && !c.complement){
                    this->setNodeType(MIG_NODE_INVINVNO);
                }
                else if(a.complement && b.complement && c.complement){
                    this->setNodeType(MIG_NODE_INVINVINV);
                }
            }
            else if(type == 1){ // Constant
                this->setNodeType(MIG_NODE_CONST1);
                _fanin0 = a.index;
                _fanin1 = b.index;
                _fanin2 = c.index;
            }
            else if(type == 2){ // Primary Input
                this->setNodeType(MIG_NODE_PI);
            }
            else if(type == 3){ // Primary Output
                this->setNodeType(MIG_NODE_PO);
                _fanin0 = a.index;
                _fanin1 = b.index;
                _fanin2 = c.index;
            }
            else if(type == 4){ // Primary Input and Output
                this->setNodeType(MIG_NODE_PIO);
            }
            else if(type == 5){ // Primary Input and Output
                this->setNodeType(MIG_NODE_POC);
            }
        }
        
        // Find what class represents a node within mockturtle
        

    private:
        IntType _fanin0 = -1; ///< The fanin 0. -1 if no fanin 0
        IntType _fanin1 = -1; ///< The fanin 1. -1 if no fanin 1
        IntType _fanin2 = -1; ///< The fanin 2. -1 if no fanin 2
        IndexType _fanout_size = -1; ///< Total fanout nodes
        IntType _nodeType = MIG_NODE_NUMBER; ///< The type of this node
};

/// @class MTL_PY::MtlInterface
/// @brief the interface to ABC
class MtlInterface
{
    public:
        explicit MtlInterface() = default;
        /*------------------------------*/ 
        /* Start and stop the framework */
        /*------------------------------*/ 
        void start();
        void end();
        /// @brief read a file
        /// @param filename
        /// @return if succesful
        float read(const std::string & filename);
        /*------------------------------*/ 
        /* Perform Logic Synthesis      */
        /*------------------------------*/
        /// @brief Perform SOP balancing on the MIG
        /// @return the time taken to perform balancing
        float balance(bool crit, IndexType cut_size); 
        /// @brief Perform rewriting on the MIG
        /// @return the time taken to perform rewriting
        float rewrite(bool allow_zero_gain, bool use_dont_cares, bool preserve_depth, IndexType min_cut_size);
        /// @brief Perform refactoring on the MIG
        /// @return the time taken to perform refactoring
        float refactor(bool allow_zero_gain, bool use_dont_cares);
        /// @brief Perform resubstitution on the MIG
        /// @return the time taken to perform resubstitution
        float resub(IndexType max_pis, IndexType max_inserts, bool use_dont_cares, IndexType window_size, bool preserve_depth);
        /*------------------------------*/ 
        /* Query the information        */
        /*------------------------------*/ 
        /// @brief get the design MIG stats from ABC
        /// @return the MIG stats from ABC
        MigStats migStats();
        /// @brief get the number of nodes (aig + PI + PO)
        /// @return the number of total nodes
        IntType numNodes()
        {
            IntType nObj = _mig.size();
            return nObj;
        }
        /// @brief update the graph
        void updateGraph();
        /// @brief Get one MigNode
        /// @param The index of MigNode
        /// @return The MigNode
        MigNode & migNode(IntType nodeIdx) 
        { 
            AssertMsg(nodeIdx < this->numNodes(), "Access node out of range %d / %d \n", nodeIdx, this->numNodes()); 
            return _migNodes[nodeIdx]; 
        }

    private:
        mockturtle::mig_network _mig;
        bool _interface = false; // To start and stop the interface
        RealType _lastClk; ///< The time of last operation
        IntType _numMigNodes = -1; ///< Number of MIG nodes
        IntType _depth = -1; ///< The depth of the MIG network
        IntType _numPI = -1; ///< Number of PIs of the MIG network
        IntType _numPO = -1; ///< Number of POs of the MIG network
        IntType _numConst = -1; ///< Number of CONST of the MIG network
        std::vector<MigNode> _migNodes; ///< The current MIG network nodes
};

PROJECT_NAMESPACE_END

#endif //MTL_PY_ABC_INTERFACE_H_

PROJECT_NAMESPACE_BEGIN

void MtlInterface::start(){
    mockturtle::mig_network mig;
    _mig = mig;
    _interface = true;
    return;
}

void MtlInterface::end(){
    _interface = false;
    return;
}

float MtlInterface::read(const std::string &filename)
{
    if(!_interface){
        return -1.0;
    }
    auto beginClk = clock();
    char Command[1000];
    // read the file
    sprintf( Command, "%s", filename.c_str() );
    lorina::read_aiger(Command, mockturtle::aiger_reader( _mig ) );
    auto endClk = clock();
    _lastClk = endClk - beginClk;
    this->updateGraph();
    return (float)_lastClk/CLOCKS_PER_SEC;
}

float MtlInterface::balance(bool crit, IndexType cut_size){
    if(!_interface){
        return -1.0;
    }
    mockturtle::sop_rebalancing<mockturtle::mig_network> sop_balancing;
    mockturtle::balancing_params ps;
    mockturtle::balancing_stats st;

    ps.cut_enumeration_ps.cut_size = cut_size;
    ps.only_on_critical_path = crit;

    _mig = mockturtle::balancing( _mig, {sop_balancing}, ps, &st );
    return mockturtle::to_seconds( st.time_total );
}

float MtlInterface::rewrite(bool allow_zero_gain, bool use_dont_cares, bool preserve_depth, IndexType min_cut_size){
    if(!_interface){
        return -1.0;
    }
    mockturtle::mig_npn_resynthesis resyn;
    mockturtle::cut_rewriting_params ps;
    mockturtle::cut_rewriting_stats st;
    ps.cut_enumeration_ps.cut_size = 4u;
    ps.min_cand_cut_size = min_cut_size;
    ps.allow_zero_gain = allow_zero_gain;
    ps.use_dont_cares = use_dont_cares;
    ps.preserve_depth = preserve_depth;
    _mig = mockturtle::cut_rewriting( _mig, resyn, ps, &st );
    _mig = mockturtle::cleanup_dangling( _mig );
    return mockturtle::to_seconds(st.time_total);
}

float MtlInterface::refactor(bool allow_zero_gain, bool use_dont_cares){
    if(!_interface){
        return -1.0;
    }
    mockturtle::akers_resynthesis<mockturtle::mig_network> resyn;
    mockturtle::refactoring_params ps;
    mockturtle::refactoring_stats st;
    ps.allow_zero_gain = allow_zero_gain;
    ps.use_dont_cares = use_dont_cares;
    mockturtle::refactoring( _mig, resyn, ps, &st);
    _mig = mockturtle::cleanup_dangling( _mig );
    return mockturtle::to_seconds(st.time_total);
}

float MtlInterface::resub(IndexType max_pis, IndexType max_inserts, bool use_dont_cares, IndexType window_size, bool preserve_depth){
    if(!_interface){
        return -1.0;
    }
    mockturtle::resubstitution_params ps;
    mockturtle::resubstitution_stats st;
    ps.max_pis = max_pis;
    ps.max_inserts = max_inserts;
    ps.use_dont_cares = use_dont_cares;
    ps.window_size = window_size;
    ps.preserve_depth = preserve_depth;
    mockturtle::depth_view _depth_mig{ _mig }; 
    mockturtle::fanout_view _fanout_mig{ _depth_mig };

    mockturtle::mig_resubstitution( _fanout_mig, ps, &st );
    _mig = mockturtle::cleanup_dangling( _mig );
    return mockturtle::to_seconds(st.time_total);
}

void MtlInterface::updateGraph()
{
    _numMigNodes = _mig.size();
    mockturtle::depth_view mig_depth{ _mig };
    _depth = mig_depth.depth();
    _numPO = _mig.num_pos();
    _numPI = _mig.num_pis();
    _numConst = 0;
    _migNodes.resize(_numMigNodes);

    std::vector <IntType> visited(_numMigNodes, 0);

    //Configure Primary Outputs
    _mig.foreach_po( [&](auto node){
        IndexType num_fanout = _mig.fanout_size(node.index);
        mockturtle::mig_network::signal ch0 = mockturtle::mig_network::signal(_mig._storage->nodes[node.index].children[0]);
        mockturtle::mig_network::signal ch1 = mockturtle::mig_network::signal(_mig._storage->nodes[node.index].children[1]);
        mockturtle::mig_network::signal ch2 = mockturtle::mig_network::signal(_mig._storage->nodes[node.index].children[2]); 
        if(node.index==0){
            // Configure as both Output and Constant
            _migNodes[node.index].configure(ch0, ch1, ch2, num_fanout, 5);    
        }
        else{
            _migNodes[node.index].configure(ch0, ch1, ch2, num_fanout, 3);
        }
        visited[node.index] = 1;
    });

    // Configure Primary Inputs
    _mig.foreach_pi( [&](auto node){
        IndexType num_fanout = _mig.fanout_size(node);
        mockturtle::mig_network::signal ch0, ch1, ch2;
        if(visited[node] != 0){
            // Configure as both Primary Input and Output
            visited[node] = 3;
            _migNodes[node].configure(ch0, ch1, ch2, num_fanout, 4);
        }
        else{
            visited[node] = 2;
            _migNodes[node].configure(ch0, ch1, ch2, num_fanout, 2);
        } 
    });

    _mig.foreach_node( [&]( auto node ){
        IndexType num_fanout = _mig.fanout_size(node);
        mockturtle::mig_network::signal ch0 = mockturtle::mig_network::signal(_mig._storage->nodes[_mig.node_to_index(node)].children[0]);
        mockturtle::mig_network::signal ch1 = mockturtle::mig_network::signal(_mig._storage->nodes[_mig.node_to_index(node)].children[1]);
        mockturtle::mig_network::signal ch2 = mockturtle::mig_network::signal(_mig._storage->nodes[_mig.node_to_index(node)].children[2]); 
        if(_mig.node_to_index(node) == 0 && !visited[0]){
            //configure as constant
            _migNodes[_mig.node_to_index(node)].configure(ch0, ch1, ch2, num_fanout, 1);             
        }
        else if(visited[_mig.node_to_index(node)] != 0 || _mig.node_to_index(node) == 0){
            // Already configured
        }
        else{
            // Configure as internal node
            _migNodes[_mig.node_to_index(node)].configure(ch0, ch1, ch2, num_fanout, 0);
            visited[_mig.node_to_index(node)] = 4;
        }
    });

}

MigStats MtlInterface::migStats()
{
    this->updateGraph();
    MigStats stats;
    stats.setNumIn(_numPI);
    stats.setNumOut(_numPO);
    stats.setNumMigNodes(_numMigNodes);
    stats.setLev(_depth);
    return stats;
}

PROJECT_NAMESPACE_END


namespace py = pybind11;
void initMtlInterfaceAPI(py::module &m)
{
    py::class_<PROJECT_NAMESPACE::MtlInterface>(m , "MtlInterface")
        .def(py::init<>())
        .def("start", &PROJECT_NAMESPACE::MtlInterface::start, "Start the interface")
        .def("end", &PROJECT_NAMESPACE::MtlInterface::end, "Deallocate space for the interface")
        .def("read", &PROJECT_NAMESPACE::MtlInterface::read, "Read a file")
        .def("migStats", &PROJECT_NAMESPACE::MtlInterface::migStats, "Get the AIG stats from the ABC framework`")
        .def("migNode", &PROJECT_NAMESPACE::MtlInterface::migNode, "Get one MigNode")
        .def("numNodes", &PROJECT_NAMESPACE::MtlInterface::numNodes, "Get the number of nodes")
        .def("balance", &PROJECT_NAMESPACE::MtlInterface::balance, "balance action",
                py::arg("crit") = false, py::arg("cut_size") = 4u)
        .def("rewrite", &PROJECT_NAMESPACE::MtlInterface::rewrite, "rewrite action",
                py::arg("allow_zero_gain") = false, py::arg("use_dont_cares") = false,
                py::arg("preserve_depth") = false, py::arg("min_cut_size") = 3u)
        .def("resub", &PROJECT_NAMESPACE::MtlInterface::resub, "resub action",
                py::arg("max_pis") = 8u, py::arg("max_inserts") = 2u, py::arg("use_dont_cares") = false,
                py::arg("window_size") = 12u, py::arg("preserve_depth") = false)
        .def("refactor", &PROJECT_NAMESPACE::MtlInterface::refactor, "refactor action",
                py::arg("allow_zero_gain") = false, py::arg("use_dont_cares") = false);

    py::class_<PROJECT_NAMESPACE::MigStats>(m , "MigStats")
        .def(py::init<>())
        .def_property("numIn", &PROJECT_NAMESPACE::MigStats::numIn, &PROJECT_NAMESPACE::MigStats::setNumIn)
        .def_property("numOut", &PROJECT_NAMESPACE::MigStats::numOut, &PROJECT_NAMESPACE::MigStats::setNumOut)
        .def_property("numLat", &PROJECT_NAMESPACE::MigStats::numLat, &PROJECT_NAMESPACE::MigStats::setNumLat)
        .def_property("numMigNodes", &PROJECT_NAMESPACE::MigStats::numMigNodes, &PROJECT_NAMESPACE::MigStats::setNumMigNodes)
        .def_property("lev", &PROJECT_NAMESPACE::MigStats::lev, &PROJECT_NAMESPACE::MigStats::setLev);

    py::class_<PROJECT_NAMESPACE::MigNode>(m, "MigNode")
        .def(py::init<>())
        .def("hasFanin0", &PROJECT_NAMESPACE::MigNode::hasFanin0, "Whether the node has fanin0")
        .def("fanin0", &PROJECT_NAMESPACE::MigNode::fanin0, "The node index of fanin 0")
        .def("hasFanin1", &PROJECT_NAMESPACE::MigNode::hasFanin1, "Whether the node has fanin1")
        .def("fanin1", &PROJECT_NAMESPACE::MigNode::fanin1, "The node index of fanin 1")
        .def("hasFanin2", &PROJECT_NAMESPACE::MigNode::hasFanin2, "Whether the node has fanin2")
        .def("fanin2", &PROJECT_NAMESPACE::MigNode::fanin2, "The node index of fanin 2")
        .def("numFanouts", &PROJECT_NAMESPACE::MigNode::numFanouts, "The number of fanouts")
        .def("nodeType", &PROJECT_NAMESPACE::MigNode::nodeType, "The node type. 0: constant, 1: PI, 2: PO, 3: abc, 4: ~abc, 5: ~a~bc, 6 ~a~b~c, 7 PI & PO, 8 PO and constant, 9 unknown");
}
