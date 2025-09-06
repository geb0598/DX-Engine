#pragma once
#include <vector>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <array>
#include <queue>
#include <string>

template <typename T> using TArray = std::vector<T>;
template <typename T> using TLinkedList = std::list<T>;
template <typename T> using TSet = std::unordered_set<T>;
template <typename K, typename V> using TMap = std::unordered_map<K, V>;
template <typename K, typename V> using TPair = std::pair<K, V>;
template <typename T, unsigned N> using TStaticArray = std::array<T, N>;
template <typename T> using TQueue = std::queue<T>;

using FString = std::string;