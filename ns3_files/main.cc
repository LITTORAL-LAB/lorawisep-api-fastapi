/*
 * This program creates a simple network which uses an ADR algorithm to set up
 * the Spreading Factors of the devices in the Network.
 */

// Includes do exemplo original (alguns já estavam)
#include "ns3/building-allocator.h"
#include "ns3/building-penetration-loss.h"
#include "ns3/buildings-helper.h"
#include "ns3/class-a-end-device-lorawan-mac.h" // Necessário para GetStatus
#include "ns3/command-line.h"
#include "ns3/constant-position-mobility-model.h" // Já estava indiretamente
#include "ns3/correlated-shadowing-propagation-loss-model.h"
#include "ns3/double.h"
#include "ns3/end-device-lora-phy.h" // Necessário para GetStatus
#include "ns3/forwarder-helper.h"
#include "ns3/gateway-lora-phy.h"
#include "ns3/gateway-lorawan-mac.h" // Necessário para GetStatus
#include "ns3/log.h"
#include "ns3/lora-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/network-server-helper.h"
#include "ns3/node-container.h"
#include "ns3/periodic-sender-helper.h"
#include "ns3/pointer.h" // Já estava indiretamente
#include "ns3/position-allocator.h" // Já estava indiretamente
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"
#include "ns3/point-to-point-module.h" // Adicionado para links P2P com NS

// Seus includes (alguns podem ser redundantes com os acima)
#include "ns3/config.h"
#include "ns3/core-module.h"
// #include "ns3/forwarder-helper.h" // já incluso
// #include "ns3/gateway-lora-phy.h" // já incluso
#include "ns3/hex-grid-position-allocator.h" // Se for usar, senão pode remover
// #include "ns3/log.h" // já incluso
#include "ns3/lora-channel.h"
#include "ns3/lora-device-address-generator.h"
// #include "ns3/lora-helper.h" // já incluso
#include "ns3/lora-phy-helper.h"
#include "ns3/lorawan-mac-helper.h"
// #include "ns3/mobility-helper.h" // já incluso
#include "ns3/network-module.h"
// #include "ns3/network-server-helper.h" // já incluso
// #include "ns3/periodic-sender-helper.h" // já incluso
#include "ns3/periodic-sender.h"
// #include "ns3/point-to-point-module.h" // já incluso
// #include "ns3/random-variable-stream.h" // já incluso
#include "ns3/rectangle.h"
#include "ns3/string.h"
// for energy
#include "ns3/basic-energy-source-helper.h"
#include "ns3/energy-module.h"
#include "ns3/file-helper.h"
#include "ns3/lora-radio-energy-model-helper.h"

// SEU UTILS.H
#include "utils.h" // Este já define OnDataRateChange e OnTxPowerChange

#include <algorithm> // Do exemplo original
#include <ctime>     // Do exemplo original
#include <vector>    // Para std::vector em vez de VLA

using namespace ns3;
using namespace lorawan;

NS_LOG_COMPONENT_DEFINE("LoRaWISEP");

// REMOVIDO: Funções OnDataRateChange e OnTxPowerChange pois estão em utils.h
// void
// OnDataRateChange(uint8_t oldDr, uint8_t newDr)
// {
//     NS_LOG_DEBUG("DR" << unsigned(oldDr) << " -> DR" << unsigned(newDr));
// }

// void
// OnTxPowerChange(double oldTxPower, double newTxPower)
// {
//     NS_LOG_DEBUG(oldTxPower << " dBm -> " << newTxPower << " dBm");
// }

// bool verbose = false; // Removido se não usado
bool adrEnabled = true;
bool initializeSF = false;
double radius = 6400; // Valor do exemplo original, mas você pode sobrescrever
bool fromfile = true; // Mantido
int numRun = 1;

// devices configuration
std::string file_endevices = "";

// gateways configuration
std::string file_gateway = "";

std::string out_folder = "";
// network configuration
bool realisticChannelModel = true; // Alterado para bool, como no exemplo original
int appPeriodSeconds = 600;
double simulationTime = 1200; // no exemplo original é simulationTimeSeconds
std::string adrType = "ns3::AdrComponent"; // Mantido, assumindo que é usado por NetworkServerHelper

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__); // Adicionado __FILE__ como no exemplo
    // cmd.AddValue ("verbose", "Whether to print output or not", verbose);
    // cmd.AddValue ("MultipleGwCombiningMethod", "ns3::AdrComponent::MultipleGwCombiningMethod");
    // cmd.AddValue ("MultiplePacketsCombiningMethod", "ns3::AdrComponent::MultiplePacketsCombiningMethod");
    // cmd.AddValue ("HistoryRange", "ns3::AdrComponent::HistoryRange");
    // cmd.AddValue ("MType", "ns3::EndDeviceLorawanMac::MType");
    // cmd.AddValue ("EDDRAdaptation", "ns3::EndDeviceLorawanMac::EnableEDDataRateAdaptation");
    // cmd.AddValue ("ChangeTransmissionPower", "ns3::AdrComponent::ChangeTransmissionPower");
    // cmd.AddValue ("AdrEnabled", "Whether to enable ADR", adrEnabled); // Pode ser adicionado se quiser controlar via cmd
    // cmd.AddValue ("nDevices", "Number of devices to simulate", nDevices); // nDevices será lido do arquivo
    cmd.AddValue("radius", "The radius (m) of the area to simulate (if not from file)", radius); // Adicionado do exemplo
    cmd.AddValue("realisticChannel", "Whether to use a more realistic channel model", realisticChannelModel); // Adicionado do exemplo
    cmd.AddValue("simulationTime", "The time (s) for which to simulate", simulationTime); // Nomeado como no exemplo (era simulationTimeSeconds)
    cmd.AddValue("appPeriod", "The period in seconds to be used by periodically transmitting applications", appPeriodSeconds); // Adicionado do exemplo

    cmd.AddValue("numRun", "Number of Running", numRun);
    cmd.AddValue("initializeSF", "Whether to initialize the SFs", initializeSF);
    cmd.AddValue("MaxTransmissions", "ns3::EndDeviceLorawanMac::MaxTransmissions");
    cmd.AddValue("file_endevices", "Path to the devices configuration file", file_endevices);
    cmd.AddValue("file_gateways", "Path to the gateways configuration file", file_gateway);
    cmd.AddValue("out_folder", "Path to the output folder", out_folder);
    // Adicionar printBuildingInfo se necessário
    // cmd.AddValue("print", "Whether or not to print building information", printBuildingInfo); // printBuildingInfo precisa ser declarada
    cmd.Parse(argc, argv);

    // Logging
    // LogComponentEnable("LoRaWISEP", LOG_LEVEL_ALL); // Nome do componente
    // LogComponentEnable("ComplexLorawanNetworkExample", LOG_LEVEL_INFO); // Usando o do exemplo original
    // ... (outros LogComponentEnable do exemplo original podem ser adicionados ou mantidos os seus)
    // LogComponentEnable("LoraChannel", LOG_LEVEL_INFO);
    // LogComponentEnable("NetworkServer", LOG_LEVEL_ALL); // Útil para depurar NS

    // Set the EDs to require Data Rate control from the NS
    // Config::SetDefault("ns3::EndDeviceLorawanMac::DRControl", BooleanValue(true));
    // Config::SetDefault ("ns3::AdrComponent::AdrEnabled", BooleanValue (adrEnabled)); // Se quiser usar AdrComponent diretamente

    ///////////////////////////////////
    NS_LOG_INFO("End Devices File: " << file_endevices);
    std::vector<EndDeviceData> endDeviceLocations;
    int nDevices = GetEndDeviceDataFromFile(endDeviceLocations, file_endevices);
    if (nDevices == 0 && !file_endevices.empty()) { // Adiciona verificação
        NS_LOG_ERROR("No end devices loaded from file: " << file_endevices << ". Exiting.");
        return 1;
    }
    NS_LOG_INFO("Loaded " << nDevices << " end device locations from " << file_endevices);

    NS_LOG_INFO("Gateways File: " << file_gateway);
    std::vector<GatewayData> gatewayLocations;
    int nGateways = GetGatewayDataFromFile(gatewayLocations, file_gateway);
     if (nGateways == 0 && !file_gateway.empty()) { // Adiciona verificação
        NS_LOG_ERROR("No gateways loaded from file: " << file_gateway << ". Exiting.");
        return 1;
    }
    NS_LOG_INFO("Loaded " << nGateways << " gateway locations from " << file_gateway);

    NS_LOG_INFO("numRun: " << numRun);
    NS_LOG_INFO("out_folder: " << out_folder);

    /***********
     *  Setup  *
     ***********/
    Time appPeriod = Seconds(appPeriodSeconds);
    NS_LOG_INFO("App Period: " << appPeriod);
    Time appStopTime = Seconds(simulationTime); // Usando a variável simulationTime
    NS_LOG_INFO("App Stop Time: " << appStopTime);

    /************************
     *  Create the channel  *
     ************************/
    Ptr<LogDistancePropagationLossModel> loss = CreateObject<LogDistancePropagationLossModel>();
    loss->SetPathLossExponent(3.76);
    loss->SetReference(1, 7.7);

    if (realisticChannelModel)
    {
        NS_LOG_INFO("Using realistic channel model");
        Ptr<CorrelatedShadowingPropagationLossModel> shadowing =
            CreateObject<CorrelatedShadowingPropagationLossModel>();
        loss->SetNext(shadowing);
        Ptr<BuildingPenetrationLoss> buildingLoss = CreateObject<BuildingPenetrationLoss>();
        shadowing->SetNext(buildingLoss);
    }

    Ptr<PropagationDelayModel> delay = CreateObject<ConstantSpeedPropagationDelayModel>();
    Ptr<LoraChannel> channel = CreateObject<LoraChannel>(loss, delay);

    /************************
     *  Create the helpers  *
     ************************/
    LoraPhyHelper phyHelper = LoraPhyHelper();
    phyHelper.SetChannel(channel);

    LorawanMacHelper macHelper = LorawanMacHelper();

    LoraHelper helper = LoraHelper();
    helper.EnablePacketTracking();

    NetworkServerHelper nsHelper = NetworkServerHelper(); // Renomeado para nsHelper como no exemplo
    ForwarderHelper forHelper = ForwarderHelper();       // Renomeado para forHelper como no exemplo

    /************************
     *  Create End Devices  *
     ************************/
    NodeContainer endDevices;
    endDevices.Create(nDevices);
    
    MobilityHelper mobilityEd; // Mobilidade específica para EDs
    if (fromfile) {
        // Se as posições vêm de arquivo, não precisamos de um alocador aleatório para EDs
        // Apenas o modelo de mobilidade constante
         mobilityEd.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    } else {
        // Configuração de mobilidade do exemplo original se não for de arquivo
        mobilityEd.SetPositionAllocator("ns3::UniformDiscPositionAllocator",
                                      "rho",
                                      DoubleValue(radius),
                                      "X",
                                      DoubleValue(0.0),
                                      "Y",
                                      DoubleValue(0.0));
        mobilityEd.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    }
    mobilityEd.Install(endDevices);


    int counter = 0;
    for (NodeContainer::Iterator j = endDevices.Begin(); j != endDevices.End(); ++j, counter++)
    {
        Ptr<MobilityModel> mobModel = (*j)->GetObject<MobilityModel>();
        Vector position; // Não pega a posição do allocator se sempre vem do arquivo
        if (fromfile && counter < nDevices) // Checagem de segurança
        {
            position.x = endDeviceLocations[counter].coordX;
            position.y = endDeviceLocations[counter].coordY;
        }
        position.z = 1.2;
        mobModel->SetPosition(position);
        // Print para depuração (opcional)
        // std::cout << "End Device " << (*j)->GetId() << " (idx " << counter << "): "
        //           << position.x << ", " << position.y << ", " << position.z << std::endl;
    }

    uint8_t nwkId = 54;
    uint32_t nwkAddr = 1864;
    Ptr<LoraDeviceAddressGenerator> addrGen =
        CreateObject<LoraDeviceAddressGenerator>(nwkId, nwkAddr);

    macHelper.SetAddressGenerator(addrGen);
    phyHelper.SetDeviceType(LoraPhyHelper::ED);
    macHelper.SetDeviceType(LorawanMacHelper::ED_A);
    NetDeviceContainer endDevicesNetDevices = helper.Install(phyHelper, macHelper, endDevices);

    // Connect trace sources (do exemplo original, pode ser útil)
    // for (auto j = endDevices.Begin(); j != endDevices.End(); ++j)
    // {
    //     Ptr<Node> node = *j;
    //     Ptr<LoraNetDevice> loraNetDevice = DynamicCast<LoraNetDevice>(node->GetDevice(0));
    //     Ptr<LoraPhy> phy = loraNetDevice->GetPhy();
    // }
    NS_LOG_DEBUG("End devices created");

    /*********************
     *  Create Gateways  *
     *********************/
    NodeContainer gateways;
    gateways.Create(nGateways);

    MobilityHelper mobilityGw; // Mobilidade específica para GWs
    Ptr<ListPositionAllocator> allocator = CreateObject<ListPositionAllocator>();
    counter = 0;
    for (NodeContainer::Iterator j = gateways.Begin(); j != gateways.End(); ++j, counter++)
    {
        double gwa = 0, gwb = 0, gwc = 15.0;
        if (fromfile && counter < nGateways)
        {
            gwa = gatewayLocations[counter].coordX;
            gwb = gatewayLocations[counter].coordY;
        } // else ... (lógica para quando não é fromfile)
        allocator->Add(Vector(gwa, gwb, gwc));
        // Print para depuração (opcional)
        // std::cout << "Gateway " << (*j)->GetId() << " (idx " << counter << "): "
        //           << gwa << ", " << gwb << ", " << gwc << std::endl;
    }
    mobilityGw.SetPositionAllocator(allocator);
    mobilityGw.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobilityGw.Install(gateways);

    phyHelper.SetDeviceType(LoraPhyHelper::GW);
    macHelper.SetDeviceType(LorawanMacHelper::GW);
    helper.Install(phyHelper, macHelper, gateways);
    NS_LOG_DEBUG("Gateways created");

    /**********************
     *  Handle buildings  *
     **********************/
    NS_LOG_DEBUG("Creating buildings");
    // Valores do exemplo original
    double xLength = 130;
    double deltaX = 32;
    double yLength = 64;
    double deltaY = 17;
    int gridWidth = 0; // Inicializa
    int gridHeight = 0; // Inicializa

    if (realisticChannelModel) // Só cria prédios se o modelo for realista
    {
        // No exemplo original, radiusMeters era usado. Aqui usamos 'radius'
        gridWidth = 2 * radius / (xLength + deltaX);
        gridHeight = 2 * radius / (yLength + deltaY);
    }
    
    BuildingContainer bContainer; // Declara fora do if para estar no escopo do 'Print buildings'
    if (realisticChannelModel && gridWidth > 0 && gridHeight > 0) // Só cria se tiver dimensões
    {
        Ptr<GridBuildingAllocator> gridBuildingAllocator;
        gridBuildingAllocator = CreateObject<GridBuildingAllocator>();
        gridBuildingAllocator->SetAttribute("GridWidth", UintegerValue(gridWidth));
        gridBuildingAllocator->SetAttribute("LengthX", DoubleValue(xLength));
        gridBuildingAllocator->SetAttribute("LengthY", DoubleValue(yLength));
        gridBuildingAllocator->SetAttribute("DeltaX", DoubleValue(deltaX));
        gridBuildingAllocator->SetAttribute("DeltaY", DoubleValue(deltaY));
        gridBuildingAllocator->SetAttribute("Height", DoubleValue(6));
        gridBuildingAllocator->SetBuildingAttribute("NRoomsX", UintegerValue(2));
        gridBuildingAllocator->SetBuildingAttribute("NRoomsY", UintegerValue(4));
        gridBuildingAllocator->SetBuildingAttribute("NFloors", UintegerValue(2));
        // Centraliza os prédios como no exemplo original
        gridBuildingAllocator->SetAttribute(
            "MinX",
            DoubleValue(-gridWidth * (xLength + deltaX) / 2 + deltaX / 2));
        gridBuildingAllocator->SetAttribute(
            "MinY",
            DoubleValue(-gridHeight * (yLength + deltaY) / 2 + deltaY / 2));
        bContainer = gridBuildingAllocator->Create(gridWidth * gridHeight);

        BuildingsHelper::Install(endDevices);
        BuildingsHelper::Install(gateways);
        NS_LOG_DEBUG("Buildings created and installed.");
    } else {
        NS_LOG_DEBUG("No buildings will be created (realisticChannelModel is false or grid dimensions are zero).");
    }


    // Print the buildings (mesmo se bContainer estiver vazio, o arquivo será criado vazio)
    // bool printBuildingInfo = true; // Defina isso ou passe por cmd
    // if (printBuildingInfo) // Adicione um if para controlar isso
    // {
        std::ofstream myfile;
        // Crie o diretório se não existir (NS-3 não faz isso automaticamente para std::ofstream)
        // Você pode precisar usar <filesystem> (C++17) ou system(mkdir -p) antes
        // Exemplo simples (pode não ser portável/seguro):
        // std::string mkdir_cmd = "mkdir -p " + out_folder + "/buildings";
        // system(mkdir_cmd.c_str());
        myfile.open(out_folder + "/buildings/buildings" + std::to_string(numRun) + ".txt");
        if (myfile.is_open()) {
            std::vector<Ptr<Building>>::const_iterator it;
            int j_build = 1; // Renomeado para não conflitar com iterador 'j'
            for (it = bContainer.Begin(); it != bContainer.End(); ++it, ++j_build)
            {
                Box boundaries = (*it)->GetBoundaries();
                myfile << "set object " << j_build << " rect from " << boundaries.xMin << "," << boundaries.yMin
                       << " to " << boundaries.xMax << "," << boundaries.yMax << std::endl;
            }
            myfile.close();
        } else {
            NS_LOG_WARN("Could not open buildings output file: " << out_folder + "/buildings/buildings" + std::to_string(numRun) + ".txt");
        }
    // }


    /**********************************************
     *  Set up the end device's spreading factor  *
     **********************************************/
    if (initializeSF)
    {
        // O exemplo original chama LorawanMacHelper::SetSpreadingFactorsUp
        // Isso é diferente de macHelper.SetSpreadingFactorsUp
        // Usando o do exemplo:
        LorawanMacHelper::SetSpreadingFactorsUp(endDevices, gateways, channel);
        NS_LOG_INFO("Spreading Factors initialized.");
    } else {
        NS_LOG_INFO("Spreading Factors will be set by ADR or default.");
    }

    // Connect our traces (já estão em utils.h, mas o exemplo conecta aqui)
    // Config::ConnectWithoutContext(
    //     "/NodeList/*/DeviceList/0/$ns3::LoraNetDevice/Mac/$ns3::EndDeviceLorawanMac/TxPower",
    //     MakeCallback(&OnTxPowerChange)); // Definido em utils.h
    // Config::ConnectWithoutContext(
    //     "/NodeList/*/DeviceList/0/$ns3::LoraNetDevice/Mac/$ns3::EndDeviceLorawanMac/DataRate",
    //     MakeCallback(&OnDataRateChange)); // Definido em utils.h


    NS_LOG_DEBUG("Completed configuration");

    /*********************************************
     *  Install applications on the end devices  *
     *********************************************/
    NS_LOG_DEBUG("Installing applications on the end devices");
    PeriodicSenderHelper appHelper = PeriodicSenderHelper();
    appHelper.SetPeriod(appPeriod); // Usa a variável Time appPeriod
    appHelper.SetPacketSize(23); // Tamanho do pacote do exemplo original
    // Ptr<RandomVariableStream> rv = // Random start time do exemplo original, se quiser
    //     CreateObjectWithAttributes<UniformRandomVariable>("Min", DoubleValue(0),
    //                                                       "Max", DoubleValue(10));
    ApplicationContainer appContainer = appHelper.Install(endDevices);

    appContainer.Start(Seconds(0)); // Pode adicionar um start aleatório com rv
    appContainer.Stop(appStopTime);
    NS_LOG_DEBUG("Applications installed");

    /**************************
     *  Create network server *
     ***************************/
    NS_LOG_DEBUG("Creating Network Server and P2P links");
    // Create the network server node (UM ÚNICO NÓ)
    Ptr<Node> networkServerNode = CreateObject<Node>(); // Renomeado para clareza

    // PointToPoint links entre gateways e servidor
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));
    
    // Store network server app registration details for later
    // (Este tipo é definido em network-server-helper.h)
    P2PGwRegistration_t gwRegistration; 
    for (auto gwNodeIt = gateways.Begin(); gwNodeIt != gateways.End(); ++gwNodeIt)
    {
        Ptr<Node> gwNode = *gwNodeIt; // Pega o Ptr<Node> do iterador
        NetDeviceContainer p2pDevices = p2p.Install(networkServerNode, gwNode);
        // O NetDevice no índice 0 é o do lado do networkServerNode
        // O NetDevice no índice 1 é o do lado do gwNode
        Ptr<PointToPointNetDevice> serverP2PNetDev = DynamicCast<PointToPointNetDevice>(p2pDevices.Get(0));
        // Precisamos do Ptr<Node> do gateway, não do iterador diretamente se emplace_back precisar
        gwRegistration.emplace_back(serverP2PNetDev, gwNode); 
    }

    // Create a network server para a rede
    nsHelper.SetGatewaysP2P(gwRegistration);
    nsHelper.SetEndDevices(endDevices);
    // Se você tem adrType e adrEnabled, pode configurar aqui, se a API suportar:
    if (adrEnabled) {
        nsHelper.EnableAdr(true); // true é o padrão, mas explícito
        // nsHelper.SetAdr(adrType); // Se quiser especificar um tipo de ADR diferente do padrão
    } else {
        nsHelper.EnableAdr(false);
    }
    nsHelper.Install(networkServerNode); // Instala no nó único do servidor
    NS_LOG_DEBUG("Network Server created and installed");

    // Create a forwarder for each gateway
    forHelper.Install(gateways);
    NS_LOG_DEBUG("Forwarders installed on gateways");

    /************************
     * Install Energy Model *
     ************************/
    BasicEnergySourceHelper basicSourceHelper;
    LoraRadioEnergyModelHelper radioEnergyHelper;

    basicSourceHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(10000));
    basicSourceHelper.Set("BasicEnergySupplyVoltageV", DoubleValue(3.3));
    radioEnergyHelper.Set("StandbyCurrentA", DoubleValue(0.0014));
    radioEnergyHelper.Set("TxCurrentA", DoubleValue(0.028));
    radioEnergyHelper.Set("SleepCurrentA", DoubleValue(0.0000015));
    radioEnergyHelper.Set("RxCurrentA", DoubleValue(0.0112));
    radioEnergyHelper.SetTxCurrentModel("ns3::LinearLoraTxCurrentModel");

    EnergySourceContainer sources = basicSourceHelper.Install(endDevices);
    // Names::Add("/Names/EnergySource", sources.Get(0)); // Cuidado se nDevices > 0 for falso
    if (nDevices > 0) { // Adiciona o nome apenas se houver dispositivos
        Names::Add("/Names/EnergySource", sources.Get(0));
    }


    DeviceEnergyModelContainer deviceModels =
        radioEnergyHelper.Install(endDevicesNetDevices, sources);

    /**********************
     * Print output files *
     ********************/
    FileHelper fileHelper; // Declarado uma vez
    NS_LOG_INFO("Configuring output files..."); // Alterado para INFO

    // Criar diretórios de saída se não existirem (necessário para NS-3 com std::ofstream)
    // Exemplo rudimentar, idealmente usar <filesystem> ou chamadas de sistema mais robustas
    // system(("mkdir -p " + out_folder + "/status").c_str());
    // system(("mkdir -p " + out_folder + "/buildings").c_str()); // Já tratado acima
    // system(("mkdir -p " + out_folder + "/phyPerformance").c_str());
    // system(("mkdir -p " + out_folder + "/globalPerformance").c_str());
    // system(("mkdir -p " + out_folder + "/battery").c_str());
    // Lembre-se que system() pode ser um risco de segurança e não é portável.

    // A função getStatus precisa ser chamada DEPOIS que a simulação configurou os SFs (e.g. por ADR ou SetSpreadingFactorsUp)
    // Chamá-la aqui pode dar valores de DR default (e.g. DR0) antes do ADR agir.
    // Para ter o status *final*, chame após Simulator::Run() ou agende com Simulator::ScheduleLast().
    // Para um status *inicial* (após possível SetSpreadingFactorsUp):
    // if (initializeSF) { // Somente se SFs foram inicializados manualmente
    //      NS_LOG_INFO("Saving initial status...");
    //      getStatus(endDevices,
    //                gateways,
    //                channel,
    //                out_folder + "/status/status_initial_" + std::to_string(numRun) + ".csv");
    // }


    helper.DoPrintDeviceStatus(endDevices, gateways, out_folder + "/ed_positions_" + std::to_string(numRun) + ".dat"); // Renomeado para clareza

    Time stateSamplePeriod = appPeriod; // Usa a variável Time
    helper.EnablePeriodicDeviceStatusPrinting(endDevices,
                                              gateways,
                                              out_folder + "/nodeData_" + std::to_string(numRun) + ".txt",
                                              stateSamplePeriod);
    helper.EnablePeriodicPhyPerformancePrinting(gateways,
                                                out_folder + "/phyPerformance/phyPerformance_" +
                                                    std::to_string(numRun) + ".csv",
                                                stateSamplePeriod);
    helper.EnablePeriodicGlobalPerformancePrinting(
        out_folder + "/globalPerformance/globalPerformance_" + std::to_string(numRun) + ".csv",
        stateSamplePeriod);

    if (nDevices > 0) { // Configura fileHelper apenas se houver dispositivos para rastrear energia
        fileHelper.ConfigureFile(out_folder + "/battery/battery-level_" + std::to_string(numRun) + ".csv", // Adicionado .csv
                                 FileAggregator::COMMA_SEPARATED);
        fileHelper.WriteProbe("ns3::DoubleProbe",
                              "/Names/EnergySource/RemainingEnergy",
                              "Output"); // Cuidado, isso rastreia apenas o source.Get(0)
    }
    

    ////////////////
    // Simulation //
    ////////////////
    Simulator::Stop(appStopTime + Hours(1)); // Como no exemplo original

    NS_LOG_INFO("Running simulation...");
    Simulator::Run();

    // Chamar getStatus aqui para obter o estado *após* a simulação (ADR, etc.)
    NS_LOG_INFO("Saving final status...");
    // getStatus(endDevices,
    //           gateways,
    //           channel,
    //           out_folder + "/status/status_final_" + std::to_string(numRun) + ".csv");

        // Salva o status final APÓS a simulação
    if (nDevices > 0 && nGateways > 0) {
        NS_LOG_INFO("Saving final network status...");
        SaveNetworkStatus(endDevices, gateways, channel, out_folder + "/status/status_final_run" + std::to_string(numRun) + ".csv");
    }


    Simulator::Destroy();

    ///////////////////////////
    // Print results to file //
    ///////////////////////////
    NS_LOG_INFO("Computing performance metrics...");
    LoraPacketTracker& tracker = helper.GetPacketTracker(); // Já estava
    // O exemplo original imprime para std::cout. Você pode redirecionar para um arquivo se quiser.
    std::cout << "Global MAC Packet Counts for run " << numRun << ":" << std::endl;
    std::cout << tracker.CountMacPacketsGlobally(Seconds(0), appStopTime + Hours(1)) << std::endl;


    return 0;
}