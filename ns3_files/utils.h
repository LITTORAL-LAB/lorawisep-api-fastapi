#ifndef UTILS_FILE_H // Alterado para evitar conflitos com outros "FILE_H"
#define UTILS_FILE_H

#include "ns3/point-to-point-module.h"
#include "ns3/forwarder-helper.h"
#include "ns3/network-server-helper.h"
#include "ns3/lora-channel.h"
#include "ns3/mobility-helper.h"
#include "ns3/lora-phy-helper.h"
#include "ns3/lorawan-mac-helper.h"
#include "ns3/lora-helper.h"
#include "ns3/gateway-lora-phy.h"
#include "ns3/periodic-sender.h"
#include "ns3/periodic-sender-helper.h"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/command-line.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/lora-device-address-generator.h"
#include "ns3/random-variable-stream.h"
#include "ns3/config.h"
#include "ns3/rectangle.h"
#include "ns3/hex-grid-position-allocator.h"
//for energy
#include "ns3/basic-energy-source-helper.h"
#include "ns3/lora-radio-energy-model-helper.h"
#include "ns3/file-helper.h"
#include "ns3/energy-module.h"
//for buildings
#include "ns3/correlated-shadowing-propagation-loss-model.h"
#include "ns3/building-penetration-loss.h"
#include "ns3/building-allocator.h"
#include "ns3/buildings-helper.h"
// #include "ns3/forwarder-helper.h" // Já incluso acima

// for geo (mantido caso você decida usá-lo para outras coisas, mas não para conversão de posições aqui)
// #include "ns3/geographic-positions.h" // Comentado, pois não estamos mais usando para conversão de coordenadas

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept> // Para std::invalid_argument, std::out_of_range
#include <cmath>     // Para log10

// NS_LOG_COMPONENT_DEFINE("UtilsLora"); // Se quiser logs específicos para este arquivo

namespace ns3 {
namespace lorawan {

// Estruturas de dados
struct EndDeviceData {
    float coordX;
    float coordY;
    // float coordZ; // Mantido comentado se não usado
    // int gateway;  // Mantido comentado se não usado ou preenchido
};

struct GatewayData {
    // int ncluster; // Mantido comentado se não usado ou preenchido
    float coordX;
    float coordY;
    // float coordZ; // Mantido comentado se não usado
};

// Retorna o número de linhas de um arquivo
int getFileLineCount(const std::string& fileName) {
    std::ifstream file(fileName);
    if (!file.is_open()) {
        // NS_LOG_ERROR("getFileLineCount: Could not open file " << fileName);
        std::cerr << "ERROR (getFileLineCount): Could not open file " << fileName << std::endl;
        return 0;
    }
    int numLines = 0;
    std::string unused;
    while (std::getline(file, unused)) {
        ++numLines;
    }
    return numLines;
}

// Lê dados de End Devices de um arquivo CSV (espera X, Y como as duas primeiras colunas)
// Retorna o número de dispositivos lidos com sucesso.
int GetEndDeviceDataFromFile(std::vector<EndDeviceData>& devices, const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        // NS_LOG_ERROR("GetEndDeviceDataFromFile: Could not open file " << filePath);
        std::cerr << "ERROR (GetEndDeviceDataFromFile): Could not open file " << filePath << std::endl;
        return 0;
    }

    std::string line;
    std::string token;
    int devicesRead = 0;
    devices.clear(); // Limpa o vetor antes de preencher

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') { // Pular linhas vazias ou comentários (opcional)
            continue;
        }
        std::stringstream ss(line);
        std::vector<std::string> tokens;
        while (std::getline(ss, token, ',')) {
            tokens.push_back(token);
        }

        if (tokens.size() >= 2) { // Precisa de pelo menos X e Y
            EndDeviceData ed;
            try {
                ed.coordX = std::stof(tokens[0]);
                ed.coordY = std::stof(tokens[1]);
                // Se tiver Z ou outros campos, leia-os aqui:
                // if (tokens.size() >= 3) ed.coordZ = std::stof(tokens[2]);
                // if (tokens.size() >= 4) ed.gateway = std::stoi(tokens[3]);
                devices.push_back(ed);
                devicesRead++;
            } catch (const std::invalid_argument& ia) {
                // NS_LOG_WARN("GetEndDeviceDataFromFile: Invalid number format in line: " << line << " (" << ia.what() << ")");
                std::cerr << "WARNING (GetEndDeviceDataFromFile): Invalid number format in line: " << line << " (" << ia.what() << ")" << std::endl;
            } catch (const std::out_of_range& oor) {
                // NS_LOG_WARN("GetEndDeviceDataFromFile: Number out of range in line: " << line << " (" << oor.what() << ")");
                std::cerr << "WARNING (GetEndDeviceDataFromFile): Number out of range in line: " << line << " (" << oor.what() << ")" << std::endl;
            }
        } else {
            // NS_LOG_WARN("GetEndDeviceDataFromFile: Insufficient columns in line: " << line);
            std::cerr << "WARNING (GetEndDeviceDataFromFile): Insufficient columns in line: " << line << std::endl;
        }
    }
    return devicesRead;
}


// Lê dados de Gateways de um arquivo CSV (espera X, Y como as duas primeiras colunas)
// Retorna o número de gateways lidos com sucesso.
int GetGatewayDataFromFile(std::vector<GatewayData>& gateways, const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        // NS_LOG_ERROR("GetGatewayDataFromFile: Could not open file " << filePath);
        std::cerr << "ERROR (GetGatewayDataFromFile): Could not open file " << filePath << std::endl;
        return 0;
    }

    std::string line;
    std::string token;
    int gatewaysRead = 0;
    gateways.clear(); // Limpa o vetor antes de preencher

    while (std::getline(file, line)) {
         if (line.empty() || line[0] == '#') { // Pular linhas vazias ou comentários
            continue;
        }
        std::stringstream ss(line);
        std::vector<std::string> tokens;
        while (std::getline(ss, token, ',')) {
            tokens.push_back(token);
        }

        if (tokens.size() >= 2) { // Precisa de pelo menos X e Y
            GatewayData gw;
            try {
                gw.coordX = std::stof(tokens[0]);
                gw.coordY = std::stof(tokens[1]);
                // Se tiver Z ou outros campos:
                // if (tokens.size() >= 3) gw.coordZ = std::stof(tokens[2]);
                // if (tokens.size() >= 4) gw.ncluster = std::stoi(tokens[3]);
                gateways.push_back(gw);
                gatewaysRead++;
            } catch (const std::invalid_argument& ia) {
                // NS_LOG_WARN("GetGatewayDataFromFile: Invalid number format in line: " << line << " (" << ia.what() << ")");
                std::cerr << "WARNING (GetGatewayDataFromFile): Invalid number format in line: " << line << " (" << ia.what() << ")" << std::endl;
            } catch (const std::out_of_range& oor) {
                // NS_LOG_WARN("GetGatewayDataFromFile: Number out of range in line: " << line << " (" << oor.what() << ")");
                std::cerr << "WARNING (GetGatewayDataFromFile): Number out of range in line: " << line << " (" << oor.what() << ")" << std::endl;
            }
        } else {
            // NS_LOG_WARN("GetGatewayDataFromFile: Insufficient columns in line: " << line);
            std::cerr << "WARNING (GetGatewayDataFromFile): Insufficient columns in line: " << line << std::endl;
        }
    }
    return gatewaysRead;
}


// Constantes para cálculo de SNR
const int LORA_BANDWIDTH_HZ = 125000;
const int LORA_NOISE_FIGURE_DB = 6;

// Calcula SNR a partir da potência de recepção (dBm)
double RxPowerToSNR(double rxPowerDbm) {
    return rxPowerDbm + 174.0 - 10.0 * std::log10(static_cast<double>(LORA_BANDWIDTH_HZ)) - static_cast<double>(LORA_NOISE_FIGURE_DB);
}

// Callbacks para mudanças de DR e TxPower
void OnDataRateChange(uint8_t oldDr, uint8_t newDr) {
    // NS_LOG_INFO("DR Change: DR" << unsigned(oldDr) << " -> DR" << unsigned(newDr));
    std::cout << Simulator::Now().GetSeconds() << "s DR Change: DR" << unsigned(oldDr) << " -> DR" << unsigned(newDr) << std::endl;
}

void OnTxPowerChange(double oldTxPower, double newTxPower) {
    // NS_LOG_INFO("TxPower Change: " << oldTxPower << " dBm -> " << newTxPower << " dBm");
    std::cout << Simulator::Now().GetSeconds() << "s TxPower Change: " << oldTxPower << " dBm -> " << newTxPower << " dBm" << std::endl;
}

// Salva o status da rede (EDs, melhor GW, métricas) em um arquivo CSV
void SaveNetworkStatus(NodeContainer endDevices,
                       NodeContainer gateways,
                       Ptr<LoraChannel> channel,
                       const std::string& outputFilePath) {
    std::ofstream outputFile(outputFilePath);
    if (!outputFile.is_open()) {
        // NS_LOG_ERROR("SaveNetworkStatus: Could not open output file " << outputFilePath);
        std::cerr << "ERROR (SaveNetworkStatus): Could not open output file " << outputFilePath << std::endl;
        return;
    }

    outputFile << "ed_id,ed_x,ed_y,ed_z,best_gw_id,gw_x,gw_y,gw_z,dr,rssi_dbm,distance_m,delay_ns,snr_db\n";

    if (gateways.GetN() == 0) {
        // NS_LOG_WARN("SaveNetworkStatus: No gateways in the container. Cannot calculate status.");
        std::cerr << "WARNING (SaveNetworkStatus): No gateways in the container. Cannot calculate status." << std::endl;
        return;
    }

    for (uint32_t i = 0; i < endDevices.GetN(); ++i) {
        Ptr<Node> edNode = endDevices.Get(i);
        Ptr<MobilityModel> edPosition = edNode->GetObject<MobilityModel>();
        if (!edPosition) continue; // Pular se não houver modelo de mobilidade

        Ptr<NetDevice> edNetDev = edNode->GetDevice(0);
        if (!edNetDev) continue;
        Ptr<LoraNetDevice> edLoraNetDev = edNetDev->GetObject<LoraNetDevice>();
        if (!edLoraNetDev) continue;
        Ptr<lorawan::LorawanMac> edMacAbs = edLoraNetDev->GetMac();
        if(!edMacAbs) continue;
        Ptr<ClassAEndDeviceLorawanMac> edMac = edMacAbs->GetObject<ClassAEndDeviceLorawanMac>();
        if (!edMac) continue;

        int currentDr = static_cast<int>(edMac->GetDataRate());

        // Encontrar o melhor gateway para este ED
        Ptr<Node> bestGatewayNode = gateways.Get(0); // Assume o primeiro como melhor inicialmente
        Ptr<MobilityModel> bestGwPosition = bestGatewayNode->GetObject<MobilityModel>();
        if (!bestGwPosition) continue;

        // Assumir uma potência de transmissão de referência para calcular o RSSI (ex: 14 dBm)
        // O valor real pode variar devido ao ADR. Este é para uma comparação consistente.
        double referenceTxPowerDbm = 14.0;
        double highestRxPowerDbm = channel->GetRxPower(referenceTxPowerDbm, edPosition, bestGwPosition);
        uint32_t bestGwId = bestGatewayNode->GetId();

        for (uint32_t gwIdx = 1; gwIdx < gateways.GetN(); ++gwIdx) {
            Ptr<Node> currentGwNode = gateways.Get(gwIdx);
            Ptr<MobilityModel> currentGwPos = currentGwNode->GetObject<MobilityModel>();
            if (!currentGwPos) continue;

            double currentRxPowerDbm = channel->GetRxPower(referenceTxPowerDbm, edPosition, currentGwPos);
            if (currentRxPowerDbm > highestRxPowerDbm) {
                highestRxPowerDbm = currentRxPowerDbm;
                bestGatewayNode = currentGwNode;
                bestGwPosition = currentGwPos;
                bestGwId = currentGwNode->GetId();
            }
        }

        double distanceToBestGw = edPosition->GetDistanceFrom(bestGwPosition);
        
        Ptr<PropagationDelayModel> delayModel = CreateObject<ConstantSpeedPropagationDelayModel>(); // Ou obtenha do canal se configurado
        Time propagationDelay = delayModel->GetDelay(edPosition, bestGwPosition);
        
        double snrAtBestGw = RxPowerToSNR(highestRxPowerDbm);

        Vector edPosVec = edPosition->GetPosition();
        Vector bestGwPosVec = bestGwPosition->GetPosition();

        outputFile << edNode->GetId() << ","
                   << edPosVec.x << "," << edPosVec.y << "," << edPosVec.z << ","
                   << bestGwId << "," // Usando o ID do nó do gateway
                   << bestGwPosVec.x << "," << bestGwPosVec.y << "," << bestGwPosVec.z << ","
                   << currentDr << ","
                   << highestRxPowerDbm << ","
                   << distanceToBestGw << ","
                   << propagationDelay.GetNanoSeconds() << ","
                   << snrAtBestGw << "\n";
    }
    outputFile.close();
}


} // namespace lorawan
} // namespace ns3

#endif // UTILS_FILE_H