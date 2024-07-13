#ifndef GOOSE_PKT_HPP
#define GOOSE_PKT_HPP

#include <optional>
#include <vector>
#include <cstdint>
#include <string>
#include <cstring>
#include <unordered_map>
#include <stdexcept>

class UtcTime {
    uint32_t seconds;
    uint32_t fraction;
public:
    UtcTime(uint32_t seconds, uint32_t fraction) : seconds(seconds) {
        this->fraction = static_cast<uint32_t>((static_cast<uint64_t>(fraction) * (1LL << 32)) / 1000000000LL);
    }
    std::vector<uint8_t> getEncoded() const {
        std::vector<uint8_t> encoded;
        
        for (int i = 3; i >= 0; --i) {
            encoded.push_back((seconds >> (i * 8)) & 0xFF);
        }
        for (int i = 3; i >= 0; --i) {
            encoded.push_back((fraction >> (i * 8)) & 0xFF);
        }
        return encoded;
    }
    static std::vector<uint8_t> staticGetEncoded(uint32_t seconds, uint32_t fraction) {
        fraction = static_cast<uint32_t>((static_cast<uint64_t>(fraction) * (1LL << 32)) / 1000000000LL);
        std::vector<uint8_t> encoded(8);
        encoded[0] = (seconds >> 24) & 0xFF;
        encoded[1] = (seconds >> 16) & 0xFF;
        encoded[2] = (seconds >> 8) & 0xFF;
        encoded[3] = seconds & 0xFF;
        encoded[4] = (fraction >> 24) & 0xFF;
        encoded[5] = (fraction >> 16) & 0xFF;
        encoded[6] = (fraction >> 8) & 0xFF;
        encoded[7] = fraction & 0xFF;
        return encoded;
    }
};

class FloatingPoint {
    std::string value;
public:
    FloatingPoint(const std::string& val) : value(val) {}
    std::vector<uint8_t> getEncoded() const {
        std::vector<uint8_t> encoded;
        encoded.insert(encoded.end(), value.begin(), value.end());
        return encoded;
    }
};

class TimeOfDay {
    std::string value;
public:
    TimeOfDay(const std::string& val) : value(val) {}
    std::vector<uint8_t> getEncoded() const {
        std::vector<uint8_t> encoded;
        encoded.insert(encoded.end(), value.begin(), value.end());
        return encoded;
    }
};

class Data {
public:
        enum class Type {
        Array,
        Structure,
        Boolean,
        BitString,
        Integer,
        Unsigned,
        FloatingPoint,
        Real,
        OctetString,
        VisibleString,
        BinaryTime,
        Bcd,
        BooleanArray,
        ObjId,
        MmsString,
        UtcTime
    };

    Type type;
    std::optional<std::vector<Data>> array;
    std::optional<std::vector<Data>> structure;
    std::optional<bool> boolean;
    std::optional<std::vector<uint8_t>> bitString;
    std::optional<int32_t> integer;
    std::optional<uint32_t> unsignedInt;
    std::optional<FloatingPoint> floatingPoint;
    std::optional<double> real;
    std::optional<std::string> octetString;
    std::optional<std::string> visibleString;
    std::optional<TimeOfDay> binaryTime;
    std::optional<int32_t> bcd;
    std::optional<std::vector<uint8_t>> booleanArray;
    std::optional<std::string> objId;
    std::optional<std::string> mmsString;
    std::optional<UtcTime> utcTime;

    Data(const Type& type){
        this->type = type;
        switch (this->type) {
            case Type::Array:
                array = std::vector<Data>();
                break;
            case Type::Structure:
                structure = std::vector<Data>();
                break;
            case Type::Boolean:
                boolean = false;
                break;
            case Type::BitString:
                bitString = std::vector<uint8_t>();
                break;
            case Type::Integer:
                integer = 0;
                break;
            case Type::Unsigned:
                unsignedInt = 0;
                break;
            case Type::FloatingPoint:
                floatingPoint = FloatingPoint("0.0");
                break;
            case Type::Real:
                real = 0.0;
                break;
            case Type::OctetString:
                octetString = "";
                break;
            case Type::VisibleString:
                visibleString = "";
                break;
            case Type::BinaryTime:
                binaryTime = TimeOfDay("00:00:00.000000");
                break;
            case Type::Bcd:
                bcd = 0;
                break;
            case Type::BooleanArray:
                booleanArray = std::vector<uint8_t>();
                break;
            case Type::ObjId:
                objId = "";
                break;
            case Type::MmsString:
                mmsString = "";
                break;
            case Type::UtcTime:
                utcTime = UtcTime(0, 0);
                break;
        }
    }

    std::vector<uint8_t> getEncoded() const {
        std::vector<uint8_t> encoded;

        switch (type) {
            case Type::Array:
                // Encode array
                encoded.push_back(0xA1);
                for (const auto& item : array.value()) {
                    std::vector<uint8_t> itemEncoded = item.getEncoded();
                    encoded.insert(encoded.end(), itemEncoded.begin(), itemEncoded.end());
                }
                break;
            case Type::Structure:
                // Encode structure
                encoded.push_back(0xA2);
                for (const auto& item : structure.value()) {
                    std::vector<uint8_t> itemEncoded = item.getEncoded();
                    encoded.insert(encoded.end(), itemEncoded.begin(), itemEncoded.end());
                }
                break;
            case Type::Boolean:
                encoded.push_back(0x83);
                encoded.push_back(1);
                encoded.push_back(boolean.value() ? 0xFF : 0x00);
                break;
            case Type::BitString:
                encoded.push_back(0x84);
                encoded.push_back(bitString.value().size());
                encoded.insert(encoded.end(), bitString.value().begin(), bitString.value().end());
                break;
            case Type::Integer:
                encoded.push_back(0x85);
                encoded.push_back(4);
                encoded.push_back((integer.value() >> 24) & 0xFF);
                encoded.push_back((integer.value() >> 16) & 0xFF);
                encoded.push_back((integer.value() >> 8) & 0xFF);
                encoded.push_back(integer.value() & 0xFF);
                break;
            case Type::Unsigned:
                encoded.push_back(0x86);
                encoded.push_back(4);
                encoded.push_back((unsignedInt.value() >> 24) & 0xFF);
                encoded.push_back((unsignedInt.value() >> 16) & 0xFF);
                encoded.push_back((unsignedInt.value() >> 8) & 0xFF);
                encoded.push_back(unsignedInt.value() & 0xFF);
                break;
            case Type::FloatingPoint:
                encoded.push_back(0x87);
                {
                    std::vector<uint8_t> fpEncoded = floatingPoint.value().getEncoded();
                    encoded.push_back(fpEncoded.size());
                    encoded.insert(encoded.end(), fpEncoded.begin(), fpEncoded.end());
                }
                break;
            case Type::Real:
                encoded.push_back(0x88);
                {
                    std::vector<uint8_t> realEncoded(4);
                    std::memcpy(realEncoded.data(), &real.value(), 4);
                    encoded.insert(encoded.end(), realEncoded.begin(), realEncoded.end());
                }
                break;
            case Type::OctetString:
                encoded.push_back(0x89);
                encoded.push_back(octetString.value().size());
                encoded.insert(encoded.end(), octetString.value().begin(), octetString.value().end());
                break;
            case Type::VisibleString:
                encoded.push_back(0x8A);
                encoded.push_back(visibleString.value().size());
                encoded.insert(encoded.end(), visibleString.value().begin(), visibleString.value().end());
                break;
            case Type::BinaryTime:
                encoded.push_back(0x8B);
                {
                    std::vector<uint8_t> btEncoded = binaryTime.value().getEncoded();
                    encoded.push_back(btEncoded.size());
                    encoded.insert(encoded.end(), btEncoded.begin(), btEncoded.end());
                }
                break;
            case Type::Bcd:
                encoded.push_back(0x8C);
                encoded.push_back(4);
                encoded.push_back((bcd.value() >> 24) & 0xFF);
                encoded.push_back((bcd.value() >> 16) & 0xFF);
                encoded.push_back((bcd.value() >> 8) & 0xFF);
                encoded.push_back(bcd.value() & 0xFF);
                break;
            case Type::BooleanArray:
                encoded.push_back(0x8D);
                encoded.push_back(booleanArray.value().size());
                encoded.insert(encoded.end(), booleanArray.value().begin(), booleanArray.value().end());
                break;
            case Type::ObjId:
                encoded.push_back(0x8E);
                encoded.push_back(objId.value().size());
                encoded.insert(encoded.end(), objId.value().begin(), objId.value().end());
                break;
            case Type::MmsString:
                encoded.push_back(0x8F);
                encoded.push_back(mmsString.value().size());
                encoded.insert(encoded.end(), mmsString.value().begin(), mmsString.value().end());
                break;
            case Type::UtcTime:
                encoded.push_back(0x90);
                {
                    std::vector<uint8_t> utcEncoded = utcTime.value().getEncoded();
                    encoded.push_back(utcEncoded.size());
                    encoded.insert(encoded.end(), utcEncoded.begin(), utcEncoded.end());
                }
                break;
        }
        return encoded;
    }
};

class IECGoose {
public:
    uint16_t appID;
    uint16_t reserved1 = 0;
    uint16_t reserved2 = 0;

    std::string gocbRef;
    int32_t timeAllowedtoLive;
    std::string datSet;
    std::optional<std::string> goID;
    UtcTime t;
    int32_t stNum;
    int32_t sqNum;
    bool simulation = false;
    int32_t confRev;
    bool ndsCom = false;
    int32_t numDatSetEntries;
    std::vector<Data> allData;

    mutable std::vector<uint8_t> encoded;
    mutable std::unordered_map<std::string, size_t> indices;

    int offSet;

    IECGoose(uint16_t appID, const std::string& gocbRef, int32_t timeAllowedtoLive,
             const std::string& datSet, const UtcTime& t,
             int32_t stNum, int32_t sqNum, int32_t confRev,
             int32_t numDatSetEntries, const std::vector<Data>& allData)
        : appID(appID), gocbRef(gocbRef), timeAllowedtoLive(timeAllowedtoLive), datSet(datSet),
          t(t), stNum(stNum), sqNum(sqNum), confRev(confRev), allData(allData) {}

    int getParamPos(const std::string& param) const {
        auto it = indices.find(param);
        return (it != indices.end()) ? (it->second + this->offSet) : -1;
    }

    std::vector<uint8_t> getEncoded() {
        encoded.clear();  // Clear previous encoding
        encoded.reserve(2048);

        std::vector<uint8_t> pduEncoded = this->getPduEncoded();

        uint16_t pduSize = pduEncoded.size();
        uint16_t length = 8 + 2 + pduSize;
        offSet = 14;

        numDatSetEntries = allData.size();

        if (pduSize > 0xff) {
            length += 2;
            offSet += 2;
        } else if (pduSize > 0x80) {
            length += 1;
            offSet += 1;
        }

        // EtherType
        encoded.push_back(0x88);
        encoded.push_back(0xb8);

        // APPID
        encoded.push_back((appID >> 8) & 0xFF);
        encoded.push_back(appID & 0xFF);

        // Length 
        encoded.push_back((length >> 8) & 0xFF);
        encoded.push_back(length & 0xFF);

        // Reserved 1
        encoded.push_back((reserved1 >> 8) & 0xFF);
        encoded.push_back(reserved1 & 0xFF);

        // Reserved 2
        encoded.push_back((reserved2 >> 8) & 0xFF);
        encoded.push_back(reserved2 & 0xFF);

        // PDU
        encoded.push_back(0x61);
        if (pduSize > 0xff) {
            encoded.push_back(0x82);
            encoded.push_back((pduSize >> 8) & 0xFF);
            encoded.push_back(pduSize & 0xFF);
        } else if (pduSize > 0x80) {
            encoded.push_back(0x81);
            encoded.push_back(pduSize & 0xFF);
        } else {
            encoded.push_back(pduSize & 0xFF);
        }
        encoded.insert(encoded.end(), pduEncoded.begin(), pduEncoded.end());

        return encoded;
    }

private:
    std::vector<uint8_t> getPduEncoded() {
        std::vector<uint8_t> _encoded;
        indices.clear(); 

        this->numDatSetEntries = allData.size();

        // gocbRef
        indices["gocbRef"] = _encoded.size();
        _encoded.push_back(0x80); // Tag [0] VisibleString
        _encoded.push_back(gocbRef.size());
        _encoded.insert(_encoded.end(), gocbRef.begin(), gocbRef.end());

        // timeAllowedtoLive
        indices["timeAllowedtoLive"] = _encoded.size();
        _encoded.push_back(0x81); // Tag [1] INTEGER
        _encoded.push_back(4);
        for (int i = 3; i >= 0; --i) {
            _encoded.push_back((timeAllowedtoLive >> (i * 8)) & 0xFF);
        }

        // datSet
        indices["datSet"] = _encoded.size();
        _encoded.push_back(0x82); // Tag [2] VisibleString
        _encoded.push_back(datSet.size());
        _encoded.insert(_encoded.end(), datSet.begin(), datSet.end());

        // goID
        if (goID) {
            indices["goID"] = _encoded.size();
            _encoded.push_back(0x83); // Tag [3] VisibleString OPTIONAL
            _encoded.push_back(goID->size());
            _encoded.insert(_encoded.end(), goID->begin(), goID->end());
        }

        // t
        indices["t"] = _encoded.size();
        _encoded.push_back(0x84); // Tag [4] UtcTime
        auto tEncoded = t.getEncoded();
        _encoded.push_back(tEncoded.size());
        _encoded.insert(_encoded.end(), tEncoded.begin(), tEncoded.end());

        // stNum
        indices["stNum"] = _encoded.size();
        _encoded.push_back(0x85); // Tag [5] INTEGER (stNum)
        _encoded.push_back(4);
        for (int i = 3; i >= 0; --i) {
            _encoded.push_back((stNum >> (i * 8)) & 0xFF);
        }

        // sqNum
        indices["sqNum"] = _encoded.size();
        _encoded.push_back(0x86); // Tag [6] INTEGER (sqNum)
        _encoded.push_back(4);
        for (int i = 3; i >= 0; --i) {
            _encoded.push_back((sqNum >> (i * 8)) & 0xFF);
        }

        // Simulation
        indices["simulation"] = _encoded.size();
        _encoded.push_back(0x87); // Tag [7] BOOLEAN (simulation)
        _encoded.push_back(1);
        _encoded.push_back(simulation ? 0xFF : 0x00);

        // confRev
        indices["confRev"] = _encoded.size();
        _encoded.push_back(0x88); // Tag [8] INTEGER (confRev)
        _encoded.push_back(4);
        for (int i = 3; i >= 0; --i) {
            _encoded.push_back((confRev >> (i * 8)) & 0xFF);
        }


        // ndscom
        indices["ndsCom"] = _encoded.size();
        _encoded.push_back(0x89); // Tag [9] BOOLEAN (ndsCom)
        _encoded.push_back(1);
        _encoded.push_back(ndsCom ? 0xFF : 0x00);

        // numDatSetEntries
        indices["numDatSetEntries"] = _encoded.size();
        _encoded.push_back(0x8A); // Tag [10] INTEGER (numDatSetEntries)
        _encoded.push_back(4);
        for (int i = 3; i >= 0; --i) {
            _encoded.push_back((numDatSetEntries >> (i * 8)) & 0xFF);
        }

        // Encode allData
        indices["allData"] = _encoded.size();
        _encoded.push_back(0xab); // Tag [11] SEQUENCE OF Data
        std::vector<uint8_t> allDataEncoded;
        for (const auto& data : allData) {
            auto dataEncoded = data.getEncoded();
            allDataEncoded.insert(allDataEncoded.end(), dataEncoded.begin(), dataEncoded.end());
        }
        _encoded.push_back(allDataEncoded.size());
        _encoded.insert(_encoded.end(), allDataEncoded.begin(), allDataEncoded.end());

        return _encoded;
    }
};

class Ethernet {
public:
    std::string macSrc;
    std::string macDst;

    Ethernet(const std::string& dst, const std::string& src) : macSrc(src), macDst(dst) {}

    std::vector<uint8_t> macStrToBytes(const std::string& mac) const{
        std::vector<uint8_t> bytes;
        for (size_t i = 0; i < mac.length(); i += 3) {
            if (i + 2 > mac.length()) {
                throw std::invalid_argument("Invalid MAC address length");
            }
            bytes.push_back(static_cast<uint8_t>(std::stoi(mac.substr(i, 2), nullptr, 16)));
        }
        return bytes;
    }

    std::vector<uint8_t> getEncoded() const {
        std::vector<uint8_t> encoded;

        std::vector<uint8_t> srcBytes = macStrToBytes(macSrc);
        std::vector<uint8_t> dstBytes = macStrToBytes(macDst);

        encoded.insert(encoded.end(), dstBytes.begin(), dstBytes.end());
        encoded.insert(encoded.end(), srcBytes.begin(), srcBytes.end());

        return encoded;
    }
};

class Virtual_LAN {
public:
    uint8_t priority;
    bool DEI;
    uint16_t ID;

    Virtual_LAN(uint8_t pri, bool dei, uint16_t id) : priority(pri), DEI(dei), ID(id) {}

    std::vector<uint8_t> getEncoded() const {
        std::vector<uint8_t> encoded;

        uint16_t tci = (priority << 13) | (DEI << 12) | ID;

        encoded.push_back(0x81);
        encoded.push_back(0x00);

        encoded.push_back((tci >> 8) & 0xFF);
        encoded.push_back(tci & 0xFF);

        return encoded;
    }
};


#endif