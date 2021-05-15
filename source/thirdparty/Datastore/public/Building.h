#pragma once

#include <rapidxml.hpp>

/// <summary>
/// Utils for building the DC from [files]
/// </summary>
namespace Datacenter {
	namespace Build {
		using namespace Structure;

		constexpr static size_t Utf16BufferSize = 4096;
		static wchar_t Utf16Buffer[Utf16BufferSize];

		template<bool bForServer, bool bFor64>
		struct DCAdaptor {
			virtual std::unique_ptr<ElementItemRaw<bForServer, bFor64>> BuildRawStructureFromFiles(const char* folderName, bool bSkippComments = false) = 0;

		protected:
			std::unique_ptr<ElementItemRaw<bForServer, bFor64>> Root;
		};

		template<bool bForServer, bool bFor64>
		struct XMLDCAdaptor : public DCAdaptor<bForServer, bFor64> {
			std::unique_ptr<ElementItemRaw<bForServer, bFor64>> BuildRawStructureFromFiles(const char* folderName, bool bSkippComments = false) override {
				std::vector<std::string> extensions = {
					".xml",
					".quest",
					".condition"
				};
				size_t MaxFileSize = 0;
				auto filesInFolder = Utils::ScanForFilesInDirectory(folderName, MaxFileSize, extensions);
				if (!filesInFolder.size()) {
					Message("No files found to build from in [%s]!", folderName);
					return nullptr;
				}

				std::vector<std::unique_ptr<ElementItemRaw<bForServer, bFor64>>> AllNodes;

				size_t length = strlen(folderName);
				for (auto& t : filesInFolder) {
					std::ifstream file = std::ifstream(t.c_str(), std::ios::binary);
					if (!file.is_open()) {
						Message("Failed to open %s!", t.c_str());
						return nullptr;
					}

					file.seekg(0, std::ios::end);
					auto CurrentFileSize = (size_t)file.tellg();
					file.seekg(0, std::ios::beg);

					auto Buffer = std::unique_ptr<char[]>(new char[CurrentFileSize + 64]);
					if (!Buffer.get()) {
						Message("Failed to allocate the readBuffer, found FileMaxSize=%lld", CurrentFileSize);
						return nullptr;
					}

					if (CurrentFileSize) {
						file.read(Buffer.get(), CurrentFileSize);
					}

					Buffer[CurrentFileSize] = '\0';

					file.close();

					t.erase(t.begin(), t.begin() + length);

					auto XmlDoc = std::make_unique<rapidxml::xml_document<>>();

					XmlDoc->parse<0>(Buffer.get());

					if (XmlDoc->first_node()->next_sibling()) {
						Message("File [%s] must contain only one root node", t.c_str());
						return nullptr;
					}

					std::unique_ptr<ElementItemRaw<bForServer, bFor64>> RawXmlResult = ParseXmlFile(XmlDoc->first_node(), t.c_str(), nullptr, bSkippComments);
					if (!RawXmlResult.get()) {
						Message("Failed to parse xml from file [%s]", t.c_str());
						return nullptr;
					}

					//cache filename here
					RawXmlResult->CachedFileName = std::wstring(t.begin(), t.end());

					AllNodes.push_back(std::move(RawXmlResult));

					XmlDoc.reset();
					Buffer.reset();
				}

				std::unique_ptr<ElementItemRaw<bForServer, bFor64>> RootNode = std::make_unique<ElementItemRaw<bForServer, bFor64>>();
				RootNode->Name = L"__root__";

				for (auto& t : AllNodes) {
					RootNode->Children.push_back(t.release());
				}

				return std::move(RootNode);
			}

		private:
			std::unique_ptr<ElementItemRaw<bForServer, bFor64>> ParseXmlFile(rapidxml::xml_node<>* node, const char* fileName, ElementItemRaw<bForServer, bFor64>* Parent, bool bSkippComments = false) {
				if (node->type() != rapidxml::node_comment &&
					node->type() != rapidxml::node_element) {
					return nullptr;
				}

				std::unique_ptr<ElementItemRaw<bForServer, bFor64>> result = nullptr;

				if (node->type() == rapidxml::node_comment) {
					if (!bSkippComments) {
						return nullptr;
					}

					result = std::make_unique<ElementItemRaw<bForServer, bFor64>>();
					result->Parent = Parent;
					result->Type = Structure::RawElementType::Comment;

					if (!_MultiByteToWideChar(node->value(), Utf16Buffer, Utf16BufferSize)) {
						Message("Failed to convert utf8[%s] comment to utf16", node->value());
						return nullptr;
					}

					result->Value = std::wstring(Utf16Buffer);
				}
				else {
					result = std::make_unique<ElementItemRaw<bForServer, bFor64>>();
					result->Parent = Parent;

					if (!_MultiByteToWideChar(node->name(), Utf16Buffer, Utf16BufferSize)) {
						Message("Failed to convert utf8[%s] element name to utf16", node->name());
						return nullptr;
					}

					result->Name = std::wstring(Utf16Buffer);

					if (node->value_size()) {
						result->IsValueElement = true;

						if (!_MultiByteToWideChar(node->value(), Utf16Buffer, Utf16BufferSize)) {
							Message("Failed to convert utf8[<%s>%s</>] element value to utf16", node->name(), node->value());
							return nullptr;
						}
						result->Value = std::wstring(Utf16Buffer);
					}

					auto attributeNode = node->first_attribute();
					while (attributeNode) {
						AttributeItemRaw newAttribute;

						if (!_MultiByteToWideChar(attributeNode->name(), Utf16Buffer, Utf16BufferSize)) {
							Message("Failed to convert utf8[<%s %s='...'></>] element attribute name to utf16", node->name(), attributeNode->name());
							return nullptr;
						}

						newAttribute.Name = std::wstring(Utf16Buffer);

						if (!strcmp(attributeNode->value(), "")) {
							newAttribute.Value = L"";
						}
						else {

							if (!_MultiByteToWideChar(attributeNode->value(), Utf16Buffer, Utf16BufferSize)) {
								Message("Failed to convert utf8[<%s %s='%s'></>] element attribute value to utf16", node->name(), attributeNode->name(), attributeNode->value());
								return nullptr;
							}

							newAttribute.Value = std::wstring(Utf16Buffer);
						}

						if constexpr (bForServer) {
							const std::wstring Path = result->BuildAttributePath(newAttribute.Name);

							auto result = GServerDCInfo.ArbiterServerType.find(Path);
							if (result == GServerDCInfo.ArbiterServerType.end()) {
								auto result2 = GServerDCInfo.WorldServerType.find(Path);
								if (result2 == GServerDCInfo.WorldServerType.end()) {
									Message(L"Failed to find attribute type in the XTD files for[%ws]", Path.c_str());
									return nullptr;
								}
							
								newAttribute.ServerType = result2->second.Type;
							}
							else {
								newAttribute.ServerType = result->second.Type;
							}
						}

						result->Attributes.push_back(std::move(newAttribute));

						attributeNode = attributeNode->next_attribute();
					}

					auto childNode = node->first_node();
					while (childNode) {
						auto childElement = ParseXmlFile(childNode, fileName, result.get(), bSkippComments);
						if (childElement.get()) {
							result->Children.push_back(childElement.release());
						}

						childNode = childNode->next_sibling();
					}
				}

				result->AddReference();
				return std::move(result);
			}
		};
	}

	using SkyalkeXMLDataAdaptor = Build::XMLDCAdaptor<true, true>;
}