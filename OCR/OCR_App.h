#pragma once
#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include <bitset>
#include <unordered_map>
#include <nfd.hpp>
#include <fstream>


class OCR_App
{
private:
	sf::RenderWindow m_window;
	sf::Texture m_canvasTexture;
	sf::Image m_canvasImage;
	sf::Event m_event;
	sf::Clock m_deltaClock;
	sf::RectangleShape m_canvasRect;

	sf::Color m_drawPixelsColor;
	sf::Color m_canvasBackgroundColor;

	sf::VertexArray m_boundingBoxVertexArray;

	const char* defaultPatternsFilePath;

	char m_character;

	char m_recognizedCharacter;
	char m_endOfLine;

	uint8_t m_bitTolerance;

	sf::Vector2f m_lastMousePositionInImage;

	bool m_fileLoadedResult;
	bool m_lastMousePositionInImageInitialized;
	bool m_isOpen_RecognitionResultModal;
	bool m_isOpen_LoadDefaultPatterns;
	bool m_isOpen_LoadFileResult;

	float m_menuWindowSizePercentage;
	float m_menuWindowWidthInPixels;

	sf::Vector2u m_binaryImageResolution;

	std::unordered_map<char, std::vector<uint64_t>> m_charactersPatterns;

	void handleEvents();


	void renderMainWindowBar();
	void renderMenuWindow();
	void renderModals();

	void configureGUI();
	void clearCanvas();

	void handleDrawing();
	void drawLine(sf::Image& targetImage, sf::Vector2f pointA, sf::Vector2f pointB);

	uint64_t generateCharacterBinaryImage();

	bool getBinaryImageCellValue(const sf::Image& image, const sf::Rect<uint32_t>& cell);

	sf::Rect<uint32_t> getRectOfCharacter() const;

	void train(uint64_t binarydata);


	char recognize(uint64_t binaryImage);

	uint16_t countDifferentBits(uint64_t a, uint64_t b) const;

	bool loadPatterns(const char* path);
	bool savePatterns(const char* path) const;

public:

	OCR_App();
	~OCR_App();

	void update();

	bool isOpen() const;
};

