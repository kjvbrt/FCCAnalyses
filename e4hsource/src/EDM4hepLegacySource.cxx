#include "EDM4hepDataSource/EDM4hepLegacySource.hxx"

// STL
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <filesystem>
#include <exception>
#include <memory>

// ROOT
#include <TFile.h>
#include <podio/CollectionBase.h>
#include <podio/Frame.h>
#include <podio/ROOTFrameReader.h>
#include <podio/ROOTLegacyReader.h>

bool loadEDM4hepLegacySource() {
  return true;
}

namespace FCCAnalyses {
  /**
   * \brief Construct the EDM4hepLegacySource from the provided file.
   */
  EDM4hepLegacySource::EDM4hepLegacySource(const std::string& filePath,
                               int nEvents) : m_nSlots{1} {
    m_filePathList.emplace_back(filePath);
    SetupInput(nEvents);
  }

  /**
   * \brief Construct the EDM4hepLegacySource from the provided file list.
   */
  EDM4hepLegacySource::EDM4hepLegacySource(const std::vector<std::string>& filePathList,
                               int nEvents) : m_nSlots{1},
                                              m_filePathList{filePathList} {
    SetupInput(nEvents);
  }

  /**
   * \brief Setup input for the EDM4hepLegacySource.
   */
  void EDM4hepLegacySource::SetupInput(int nEvents) {
    // std::cout << "EDM4hepLegacySource: Constructing the source ..." << std::endl;

    if (m_filePathList.empty()) {
      throw std::runtime_error("EDM4hepLegacySource: No input files provided!");
    }

    for (const auto& filePath : m_filePathList) {
      // Check if file exists
      if (!std::filesystem::exists(filePath)) {
        throw std::runtime_error("EDM4hepLegacySource: Provided file \"" + filePath
                                 + "\" does not exist!");
      }

      // Check if the provided file contains required metadata
      TFile infile = TFile(filePath.data(), "READ");
      auto legacyMetadata = infile.Get("metadata");
      infile.Close();
      if (!legacyMetadata) {
        std::string errMsg = "EDM4hepLegacySource: ";
        errMsg += "Provided file is missing podio metadata!\n";
        errMsg += "                   ";
        errMsg += filePath.data();
        throw std::runtime_error(errMsg);
      }
    }

    // Create probing frame
    podio::Frame frame;
    unsigned int nEventsInFiles = 0;
    // std::cout << "EDM4hepLegacySource: Reading EDM4hep files in legacy mode..."
    //           << std::endl;
    podio::ROOTLegacyReader podioLegacyReader;

    podioLegacyReader.openFiles(m_filePathList);
    nEventsInFiles = podioLegacyReader.getEntries("events");
    frame = podio::Frame(podioLegacyReader.readEntry("events", 0));

    // Determine over how many events to run
    if (nEventsInFiles < 1) {
      throw std::runtime_error("EDM4hepLegacySource: No events found!");
    }
    // else {
      // std::cout << "EDM4hepLegacySource: Found " << nEventsInFiles
      //           << " events in files: \n";
      // for (const auto& filePath : m_filePathList) {
      //   std::cout << "               - " << filePath << "\n";
      // }
    // }

    if (nEvents < 0) {
      m_nEvents = nEventsInFiles;
    } else if (nEvents == 0) {
      throw std::runtime_error(
          "EDM4hepLegacySource: Requested to run over zero events!");
    } else {
      m_nEvents = nEvents;
    }
    if (nEventsInFiles < m_nEvents) {
      m_nEvents = nEventsInFiles;
    }

    // std::cout << "EDM4hepLegacySource: Running over " << m_nEvents << " events."
    //           << std::endl;

    // Get collections stored in the files
    std::vector<std::string> collNames = frame.getAvailableCollections();
    // std::cout << "EDM4hepLegacySource: Found following collections:\n";
    for (auto& collName: collNames) {
      const podio::CollectionBase* coll = frame.get(collName);
      if (coll->isValid()) {
        m_columnNames.emplace_back(collName);
        m_columnTypes.emplace_back(coll->getValueTypeName());
    //     std::cout << "                - " << collName << "\n";
      }
    }
  }


  /**
   * \brief Inform the EDM4hepLegacySource of the desired level of parallelism.
   */
  void
  EDM4hepLegacySource::SetNSlots(unsigned int nSlots) {
    // std::cout << "EDM4hepLegacySource: Setting num. of slots to: " << nSlots << std::endl;
    m_nSlots = nSlots;

    if (m_nSlots > m_nEvents) {
      throw std::runtime_error("EDM4hepLegacySource: Number of events too small!");
    }

    int eventsPerSlot = m_nEvents / m_nSlots;
    for (size_t i = 0; i < (m_nSlots - 1); ++i) {
      m_rangesAll.emplace_back(eventsPerSlot * i, eventsPerSlot * (i + 1));
    }
    m_rangesAll.emplace_back(eventsPerSlot * (m_nSlots - 1), m_nEvents);
    m_rangesAvailable = m_rangesAll;

    // Initialize set of addresses needed
    m_Collections.resize(
        m_columnNames.size(),
        std::vector<const podio::CollectionBase*>(m_nSlots, nullptr));

    // Initialize podio readers
    for (size_t i = 0; i < m_nSlots; ++i) {
      m_podioReaders.emplace_back(std::make_unique<podio::ROOTLegacyReader>());
    }

    for (size_t i = 0; i < m_nSlots; ++i) {
      m_podioReaders[i]->openFiles(m_filePathList);
    }

    // Initialize frames
    for (size_t i = 0; i < m_nSlots; ++i) {
      m_frames.emplace_back(
          std::make_unique<podio::Frame>(
              podio::Frame(m_podioReaders[i]->readEntry("events", 0))));
    }
  }


  /**
   * \brief Inform RDataSource that an event-loop is about to start.
   */
  void
  EDM4hepLegacySource::Initialize() {
    // std::cout << "EDM4hepLegacySource: Initializing the source ..."
    //           << std::endl;

  }


  /**
   * \brief Retrieve from EDM4hepLegacySource a set of ranges of entries that can be
   *        processed concurrently.
   */
  std::vector<std::pair<ULong64_t, ULong64_t>>
  EDM4hepLegacySource::GetEntryRanges() {
    // std::cout << "EDM4hepLegacySource: Getting entry ranges ..." << std::endl;

    std::vector<std::pair<ULong64_t, ULong64_t>> rangesToBeProcessed;
    for (auto& range: m_rangesAvailable) {
      rangesToBeProcessed.emplace_back(
          std::pair<ULong64_t, ULong64_t>{range.first, range.second}
      );
      if (rangesToBeProcessed.size() >= m_nSlots) {
        break;
      }
    }

    if (m_rangesAvailable.size() > m_nSlots) {
      m_rangesAvailable.erase(m_rangesAvailable.begin(),
                              m_rangesAvailable.begin() + m_nSlots);
    } else {
      m_rangesAvailable.erase(m_rangesAvailable.begin(),
                              m_rangesAvailable.end());
    }


    // std::cout << "EDM4hepLegacySource: Ranges to be processed:\n";
    // for (auto& range: rangesToBeProcessed) {
    //   std::cout << "               {" << range.first << ", " << range.second
    //             << "}\n";
    // }

    // if (m_rangesAvailable.size() > 0) {

    //   std::cout << "EDM4hepLegacySource: Ranges remaining:\n";
    //   for (auto& range: m_rangesAvailable) {
    //     std::cout << "               {" << range.first << ", " << range.second
    //               << "}\n";
    //   }
    // } else {
    //   std::cout << "EDM4hepLegacySource: No more remaining ranges.\n";
    // }

    return rangesToBeProcessed;
  }


  /**
   * \brief Inform EDM4hepLegacySource that a certain thread is about to start working
   *        on a certain range of entries.
   */
  void
  EDM4hepLegacySource::InitSlot(unsigned int slot, ULong64_t firstEntry) {
    // std::cout << "EDM4hepLegacySource: Initializing slot: " << slot
    //           << " with first entry " << firstEntry << std::endl;
  }


  /**
   * \brief Inform EDM4hepLegacySource that a certain thread is about to start working
   *        on a certain entry.
   */
  bool
  EDM4hepLegacySource::SetEntry(unsigned int slot, ULong64_t entry) {
    // std::cout << "EDM4hepLegacySource: In slot: " << slot << ", setting entry: "
    //           << entry << std::endl;

    m_frames[slot] = std::make_unique<podio::Frame>(
        podio::Frame(m_podioReaders[slot]->readEntry("events", entry)
    ));

    for (auto& collectionIndex: m_activeCollections) {
      m_frames_mutex.lock();
      const podio::CollectionBase* coll = m_frames[slot]->get(m_columnNames.at(collectionIndex));
      m_frames_mutex.unlock();
      m_Collections[collectionIndex][slot] = coll;
      // std::cout << "CollName: " << m_columnNames.at(collectionIndex) << "\n";
      // std::cout << "Address: " << m_Collections[collectionIndex][slot] << "\n";
      // std::cout << "Coll size: " << m_Collections[collectionIndex][slot]->size() << "\n";
      // if (m_Collections[collectionIndex][slot]->isValid()) {
      //   std::cout << "Collection valid\n";
      // }
    }

    return true;
  }


  /**
   * \brief Inform EDM4hepLegacySource that a certain thread finished working on a
   *        certain range of entries.
   */
  void
  EDM4hepLegacySource::FinalizeSlot(unsigned int slot) {
    // std::cout << "EDM4hepLegacySource: Finalizing slot: " << slot << std::endl;
    // std::cout << "Reader: " << &m_podioReaderRefs[slot].get() << std::endl;

    // for (auto& collectionIndex: m_activeCollections) {
    //   std::cout << "CollName: " << m_columnNames.at(collectionIndex) << "\n";
    //   std::cout << "Address: " << m_Collections[collectionIndex][slot] << "\n";
    //   if (m_Collections[collectionIndex][slot]->isValid()) {
    //     std::cout << "Collection valid\n";
    //   }
    //   std::cout << "Coll size: " << m_Collections[collectionIndex][slot]->size() << "\n";
    // }
  }


  /**
   * \brief Inform RDataSource that an event-loop finished.
   */
  void
  EDM4hepLegacySource::Finalize() {
    // std::cout << "EDM4hepLegacySource: Finalizing ..." << std::endl;
  }


  /**
   * \brief Type-erased vector of pointers to pointers to column values --- one
   *        per slot 
   */
  Record_t
  EDM4hepLegacySource::GetColumnReadersImpl(std::string_view columnName,
                                      const std::type_info& typeInfo) {
    // std::cout << "EDM4hepLegacySource: Getting column reader implementation for column:\n"
    //           << "               " << columnName
    //           << "\n               with type: " << typeInfo.name() << std::endl;

    auto itr = std::find(m_columnNames.begin(), m_columnNames.end(),
                         columnName);
    if (itr == m_columnNames.end()) {
      std::string errMsg = "EDM4hepLegacySource: Can't find requested column \"";
      errMsg += columnName;
      errMsg += "\"!";
      throw std::runtime_error(errMsg);
    }
    auto columnIndex = std::distance(m_columnNames.begin(), itr);
    m_activeCollections.emplace_back(columnIndex);
    // std::cout << "EDM4hepLegacySource: Active collections so far:\n"
    //           << "               ";
    // for (auto& i: m_activeCollections) {
    //   std::cout << i << ", ";
    // }
    // std::cout << std::endl;

    Record_t columnReaders(m_nSlots);
    for (size_t slotIndex = 0; slotIndex < m_nSlots; ++slotIndex) {
      // std::cout << "               Column index: " << columnIndex << "\n";
      // std::cout << "               Slot index: " << slotIndex << "\n";
      // std::cout << "               Address: "
      //         << &m_Collections[columnIndex][slotIndex]
      //         << std::endl;
      columnReaders[slotIndex] = (void*) &m_Collections[columnIndex][slotIndex];
    }

    return columnReaders;
  }


  /**
   * \brief Returns a reference to the collection of the dataset's column names 
   */
  const std::vector<std::string>&
  EDM4hepLegacySource::GetColumnNames() const {
    // std::cout << "EDM4hepLegacySource: Looking for column names" << std::endl;

    return m_columnNames;
  }

  /**
   * \brief Checks if the dataset has a certain column.
   */
  bool
  EDM4hepLegacySource::HasColumn(std::string_view columnName) const {
    // std::cout << "EDM4hepLegacySource: Looking for column: " << columnName
    //           << std::endl;

    if (std::find(m_columnNames.begin(),
                  m_columnNames.end(),
                  columnName) != m_columnNames.end()) {
      return true;
    }

    return false;
  }


  /**
   * \brief Type of a column as a string. Required for JITting.
   */
  std::string
  EDM4hepLegacySource::GetTypeName(std::string_view columnName) const {
    // std::cout << "EDM4hepLegacySource: Looking for type name of column: "
    //           << columnName << std::endl;

    auto itr = std::find(m_columnNames.begin(), m_columnNames.end(),
                         columnName);
    if (itr != m_columnNames.end()) {
      auto i = std::distance(m_columnNames.begin(), itr);
      // std::cout << "EDM4hepLegacySource: Found type name: "
      //           << m_columnTypes.at(i) << std::endl;

      return m_columnTypes.at(i) + "Collection";
    }

    return "float";
  }

  ROOT::RDataFrame
  FromEDM4hepLegacy(const std::vector<std::string>& filePathList) {
    ROOT::RDataFrame rdf(std::make_unique<EDM4hepLegacySource>(filePathList));

    return rdf;
  }
}