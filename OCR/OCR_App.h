#pragma once
#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include <bitset>
#include <unordered_map>
#include <nfd.hpp>
#include <fstream>
#include <iostream>
#include <imgui_internal.h>

#define BINARY_IMAGE_WIDTH 10
#define BINARY_IMAGE_HEIGHT 10
#define MAX_INCONSISTENT_BITS 25

typedef std::bitset<BINARY_IMAGE_WIDTH* BINARY_IMAGE_HEIGHT> binaryImageType;

class OCR_App
{
private:

	enum class WindowMenuTab
	{
		Recognize,
		Add
	};

	sf::RenderWindow m_window;
	sf::Texture m_canvasTexture;
	sf::Image m_canvasImage;
	sf::Event m_event;
	sf::Clock m_deltaClock;
	sf::RectangleShape m_canvasRect;

	sf::Color m_drawPixelsColor;
	sf::Color m_canvasBackgroundColor;

	sf::VertexArray m_boundingBoxVertexArray;

	WindowMenuTab m_currentTab;

	const char* defaultPatternsFilePath;


	char m_patternsWindowSelectedCharacter;
	char m_character;

	char m_recognizedCharacter;
	char m_endOfLine;

	uint8_t m_bitTolerance;

	sf::Vector2f m_lastMousePositionInImage;
	ImVec2 m_binaryImagePreviewButtonSize;

	bool m_fileLoadedResult;
	bool m_lastMousePositionInImageInitialized;
	bool m_isOpen_RecognitionResultModal;
	bool m_isOpen_LoadDefaultPatterns;
	bool m_isOpen_LoadFileResult;
	bool m_isOpen_PatternsWindow;
	bool m_isOpen_StyleSettingsWindow;

	float m_menuWindowSizePercentage;
	float m_menuWindowWidthInPixels;

	std::unordered_map<char, std::vector<binaryImageType>> m_charactersPatterns;



	static ImU32 binaryImageViewColor;
	static ImU32 binaryImageViewBackgroundColor;








	void handleEvents();


	void renderMainWindowBar();
	void renderMenuWindow();
	void renderPatternsWindow();
	void renderStyleSettingsWindow();
	void renderModals();

	void binaryImagePreview(const binaryImageType& image, const ImVec2& size);
	bool binaryImageButton(const char* id, const binaryImageType& image, const ImVec2& size);

	void configureGUI();
	void clearCanvas();

	void handleDrawing();
	void drawLine(sf::Image& targetImage, sf::Vector2f pointA, sf::Vector2f pointB);

	binaryImageType generateCharacterBinaryImage();

	bool getBinaryImageCellValue(const sf::Image& image, const sf::Rect<uint32_t>& cell);

	sf::Rect<uint32_t> getRectOfCharacter() const;

	char recognize(const binaryImageType& binaryImage);

	uint32_t countInconsistentBits(const binaryImageType& a, const binaryImageType& b) const;

	bool loadPatterns(const char* path);
	bool savePatterns(const char* path) const;

public:

	OCR_App();
	~OCR_App();

	void update();

	bool isOpen() const;
};

