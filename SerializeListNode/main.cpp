#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <unordered_map>


struct ListNode {
    ListNode* prev = nullptr;
    ListNode* next = nullptr;
    ListNode* rand = nullptr;
    std::string data;

    ListNode(const std::string& d) : prev(nullptr), next(nullptr), rand(nullptr), data(d) {}
};



void deleteList(ListNode* head) {
    while (head) {
        ListNode* nxt = head->next;
        delete head;
        head = nxt;
    }
}

ListNode* buildListFromText(const std::string& textPath,
    std::vector<ListNode*>& outNodes,
    std::vector<int>& randIndices) {
    std::ifstream fin(textPath);
    if (!fin) {
        std::cerr << "Не удалось открыть входной файл: " << textPath << "\n";
        return nullptr;
    }

    outNodes.clear();
    randIndices.clear();

    std::string line;
    while (std::getline(fin, line)) {
        if (line.empty()) continue;
        auto pos = line.find(';');
        if (pos == std::string::npos) {
            std::cerr << "Неправильный формат строки: " << line << "\n";
            for (auto* node : outNodes) {
                delete node;
            }
            outNodes.clear();
            randIndices.clear();
            return nullptr;
        }

        std::string data = line.substr(0, pos);
        std::string randStr = line.substr(pos + 1);

        int randIndex = 0;
        try {
            randIndex = std::stoi(randStr);
        }
        catch (...) {
            std::cerr << "Не удалось разобрать rand_index: " << randStr << "\n";
            return nullptr;
        }

        ListNode* node = new ListNode(data);
        outNodes.push_back(node);
        randIndices.push_back(randIndex);
    }

    for (size_t i = 1; i < outNodes.size(); ++i) {
        outNodes[i - 1]->next = outNodes[i];
        outNodes[i]->prev = outNodes[i - 1];
    }

    for (size_t i = 0; i < outNodes.size(); ++i) {
        int idx = (i < randIndices.size()) ? randIndices[i] : -1;
        if (idx >= 0 && static_cast<size_t>(idx) < outNodes.size()) {
            outNodes[i]->rand = outNodes[idx];
        }
        else {
            outNodes[i]->rand = nullptr;
        }
    }

    return outNodes.empty() ? nullptr : outNodes.front();
}

bool serializeToBinaryNextOrder(const std::string& binPath, ListNode* head) {
    std::vector<ListNode*> ordered;
    for (ListNode* cur = head; cur; cur = cur->next) ordered.push_back(cur);

    uint32_t n = static_cast<uint32_t>(ordered.size());
    std::ofstream fout(binPath, std::ios::binary);
    if (!fout) {
        std::cerr << "Не удалось открыть выходной бинарный файл: " << binPath << "\n";
        return false;
    }

    fout.write(reinterpret_cast<const char*>(&n), sizeof(n));

    std::unordered_map<ListNode*, uint32_t> pos;
    pos.reserve(n * 2);
    for (uint32_t i = 0; i < n; ++i) pos[ordered[i]] = i;

    for (uint32_t i = 0; i < n; ++i) {
        const ListNode* node = ordered[i];
        const std::string& s = node->data;
        uint32_t len = static_cast<uint32_t>(s.size());

        fout.write(reinterpret_cast<const char*>(&len), sizeof(len));
        fout.write(s.data(), len);

        int32_t randIndex = -1;
        if (node->rand) {
            auto it = pos.find(node->rand);
            if (it != pos.end()) randIndex = static_cast<int32_t>(it->second);
        }
        fout.write(reinterpret_cast<const char*>(&randIndex), sizeof(randIndex));
    }

    return true;
}

ListNode* deserializeFromBinary(const std::string& binPath, std::vector<ListNode*>& outNodes) {
    std::ifstream fin(binPath, std::ios::binary);
    if (!fin) {
        std::cerr << "Не удалось открыть входной файл: " << binPath << "\n";
        return nullptr;
    }

    uint32_t n;
    fin.read(reinterpret_cast<char*>(&n), sizeof(n));

    outNodes.clear();
    outNodes.reserve(n);
    std::vector<int> randIndices(n);

    for (uint32_t i = 0; i < n; ++i) {
        uint32_t len;
        fin.read(reinterpret_cast<char*>(&len), sizeof(len));

        std::string data(len, '\0');
        fin.read(&data[0], len);

        ListNode* node = new ListNode(data);
        outNodes.push_back(node);

        int32_t ri;
        fin.read(reinterpret_cast<char*>(&ri), sizeof(ri));
        randIndices[i] = ri;
    }

    for (uint32_t i = 1; i < n; ++i) {
        outNodes[i - 1]->next = outNodes[i];
        outNodes[i]->prev = outNodes[i - 1];
    }

    for (uint32_t i = 0; i < n; ++i) {
        int ri = randIndices[i];
        if (ri >= 0 && static_cast<size_t>(ri) < outNodes.size()) {
            outNodes[i]->rand = outNodes[ri];
        }
        else {
            outNodes[i]->rand = nullptr;
        }
    }

    return outNodes.empty() ? nullptr : outNodes.front();
}

int main() {
    setlocale(LC_ALL, "ru");

    std::vector<ListNode*> nodes;
    std::vector<int> randIdx;
    ListNode* head = buildListFromText("inlet.in", nodes, randIdx);
    if (!head) {
        std::cerr << "Ошибка построения списка из входного файла.\n";
        return 1;
    }

    if (!serializeToBinaryNextOrder("outlet.out", head)) {
        std::cerr << "Ошибка сериализации в бинарный файл.\n";
        deleteList(head);
        return 1;
    }

    std::vector<ListNode*> restored;
    ListNode* restoredHead = deserializeFromBinary("outlet.out", restored);
    if (restoredHead) {
        std::cout << "Успешно десериализовано " << restored.size() << " узла(ов) \n";

        std::cout << "Список (next): ";
        for (ListNode* cur = restoredHead; cur != nullptr; cur = cur->next) {
            std::cout << cur->data;
            if (cur->next) std::cout << " -> ";
        }
        std::cout << "\n";

        deleteList(restoredHead);
    }

    return 0;
}
