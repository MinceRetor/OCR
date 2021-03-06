#include "OCR_App.h"


nfdfilteritem_t characterPatternsFileFilters[1] = { { "Character Patterns File", "cpf" } };
uint16_t characterPatternsFileFiltersCount = 1;


OCR_App::OCR_App() :
    m_window(sf::VideoMode(800, 600), "OCR"),
    m_menuWindowSizePercentage(35),
    m_drawPixelsColor(sf::Color::Black),
    m_canvasBackgroundColor(sf::Color::White),
    m_lastMousePositionInImageInitialized(false),
    m_character('0'),
    m_endOfLine(0),
    m_recognizedCharacter(0),
    m_bitTolerance(15),
    defaultPatternsFilePath("defaultPatterns.cpf"),
    m_isOpen_RecognitionResultModal(false),
    m_isOpen_LoadDefaultPatterns(true)
{
    m_window.setFramerateLimit(60);
    ImGui::SFML::Init(m_window);

    clearCanvas();

    configureGUI();
}

OCR_App::~OCR_App()
{
    ImGui::SFML::Shutdown();
}

void OCR_App::handleEvents()
{
    while (m_window.pollEvent(m_event))
    {
        ImGui::SFML::ProcessEvent(m_window, m_event);

        switch (m_event.type)
        {
            case sf::Event::Closed:
            {
                m_window.close();
                break;
            }
            case sf::Event::Resized:
            {
                sf::FloatRect visibleArea(0, 0, m_event.size.width, m_event.size.height);
                m_window.setView(sf::View(visibleArea));

                m_boundingBoxVertexArray.clear();

                configureGUI();
                break;
            }
        }
    }
}

void OCR_App::renderMainWindowBar()
{
    if(ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Patterns"))
        {
            if (ImGui::Selectable("Clear Paterns"))
            {
                m_charactersPatterns.clear();
            }
            if (ImGui::Selectable("Load Paterns"))
            {
                NFD::UniquePath path;
                if (NFD::OpenDialog(path, characterPatternsFileFilters, characterPatternsFileFiltersCount) == NFD_OKAY)
                {
                    m_fileLoadedResult = loadPatterns(path.get());
                    m_isOpen_LoadFileResult = true;
                }
            }
            if (ImGui::Selectable("Save Patterns"))
            {
                NFD::UniquePath path;
                if (NFD::SaveDialog(path, characterPatternsFileFilters, characterPatternsFileFiltersCount) == NFD_OKAY)
                {
                    savePatterns(path.get());
                }
            }

            ImGui::EndMenu();
        }


        ImGui::EndMainMenuBar();
    }
}

void OCR_App::renderMenuWindow()
{
    ImGui::SetNextWindowSize(ImVec2(m_menuWindowWidthInPixels, m_window.getSize().y));
    ImGui::SetNextWindowPos(ImVec2(m_window.getSize().x - m_menuWindowWidthInPixels, 0));
    ImGui::Begin("##MenuWindow", nullptr, ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);


    std::string characterLabel;
    characterLabel.resize(1);
    characterLabel[0] = m_character;

    if (ImGui::BeginCombo("Character", characterLabel.c_str()))
    {
        for (char character = '0'; character <= '9'; character++)
        {
            characterLabel[0] = character;

            if (ImGui::Selectable(characterLabel.c_str()))
            {
                m_character = character;
            }
        }

        ImGui::EndCombo();
    }

    ImGui::SetNextItemWidth(ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x);
    if (ImGui::Button("Clear Canvas"))
    {
        clearCanvas();
    }

    ImGui::SetNextItemWidth(ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x);
    if (ImGui::Button("Show Character Bounding Box"))
    {
        sf::Rect<uint32_t> m_rect = getRectOfCharacter();
        //std::cout << m_rect.left << " | " << m_rect.top << " | " << m_rect.width << " | " << m_rect.height << std::endl;


        sf::Vector2f canvasPosition = m_canvasRect.getPosition();

        m_boundingBoxVertexArray.clear();
        m_boundingBoxVertexArray.resize(5);
        m_boundingBoxVertexArray[0] = sf::Vertex(sf::Vector2f(canvasPosition.x + m_rect.left, canvasPosition.y + m_rect.top), sf::Color::Red);
        m_boundingBoxVertexArray[1] = sf::Vertex(sf::Vector2f(canvasPosition.x + m_rect.left + m_rect.width, canvasPosition.y + m_rect.top), sf::Color::Red);
        m_boundingBoxVertexArray[2] = sf::Vertex(sf::Vector2f(canvasPosition.x + m_rect.left + m_rect.width, canvasPosition.y + m_rect.top + m_rect.height), sf::Color::Red);
        m_boundingBoxVertexArray[3] = sf::Vertex(sf::Vector2f(canvasPosition.x + m_rect.left, canvasPosition.y + m_rect.top + m_rect.height), sf::Color::Red);
        m_boundingBoxVertexArray[4] = sf::Vertex(sf::Vector2f(canvasPosition.x + m_rect.left, canvasPosition.y + m_rect.top), sf::Color::Red);
    }

    ImGui::SetNextItemWidth(ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x);
    /*
    if (ImGui::Button("Binaryzuj"))
    {
        sf::Rect<uint32_t> m_rect = getRectOfCharacter();

        std::bitset<64> bitset = generateCharacterBinaryImage();

        std::cout << bitset << std::endl;
    }
    */
    if (ImGui::Button("Recognize"))
    {
        m_recognizedCharacter = recognize(generateCharacterBinaryImage());

        m_isOpen_RecognitionResultModal = true;
    }

    ImGui::NewLine();
    ImGui::NewLine();
    ImGui::NewLine();
    ImGui::NewLine();
    ImGui::NewLine();

    if (ImGui::Button("Add Pattern For Current Character"))
    {
        m_charactersPatterns[m_character].push_back(generateCharacterBinaryImage());
    }

    ImGui::End();
}

void OCR_App::renderModals()
{
    if (m_isOpen_RecognitionResultModal)
    {
        ImGui::OpenPopup("RocognitionResult");
    }

    if (m_isOpen_LoadDefaultPatterns)
    {
        ImGui::OpenPopup("LoadDefaultPatterns");
    }

    if (m_isOpen_LoadFileResult)
    {
        ImGui::OpenPopup("LoadFileResult");
    }

    if (ImGui::BeginPopupModal("RocognitionResult", &m_isOpen_RecognitionResultModal, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (m_recognizedCharacter != 0)
        {
            ImGui::Text("Recognized character is: ");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(sf::Color::Red.r, sf::Color::Red.g, sf::Color::Red.b, 255), &m_recognizedCharacter);
        }
        else
        {
            ImGui::Text("Character could not be recognized.");
        }


        if (ImGui::IsMouseClicked(ImGuiMouseButton_::ImGuiMouseButton_Left))
        {
            m_isOpen_RecognitionResultModal = false;
        }

        ImGui::EndPopup();
    }


    if (ImGui::BeginPopupModal("LoadDefaultPatterns", &m_isOpen_LoadDefaultPatterns, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_AlwaysAutoResize))
    {
        static int loadedDefaultPatternsStatus = -1;
        ImGui::Text("Do you want to load default patterns?");

        if (loadedDefaultPatternsStatus != -1)
        {
            if (ImGui::Button("Close##LoadDefaultPatterns"))
            {
                m_isOpen_LoadDefaultPatterns = false;
            }
        }
        else
        {
            if (ImGui::Button("Yes##LoadDefaultPatterns"))
            {
                loadedDefaultPatternsStatus = loadPatterns(defaultPatternsFilePath);
            }

            ImGui::SameLine();
            if (ImGui::Button("No##LoadDefaultPatterns"))
            {
                m_isOpen_LoadDefaultPatterns = false;
            }
        }


        if (loadedDefaultPatternsStatus != -1)
        {
            if (loadedDefaultPatternsStatus)
            {
                ImGui::TextColored(ImVec4(sf::Color::Green.r, sf::Color::Green.g, sf::Color::Green.b, 255), "Successfully loaded default patterns.");
            }
            else
            {
                ImGui::TextColored(ImVec4(sf::Color::Red.r, sf::Color::Red.g, sf::Color::Red.b, 255), "Failed to load default patterns.");
            }

        }

        ImGui::EndPopup();
    }


    if (ImGui::BeginPopupModal("LoadFileResult", &m_isOpen_LoadFileResult, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (m_fileLoadedResult)
        {
            ImGui::TextColored(ImVec4(sf::Color::Green.r, sf::Color::Green.g, sf::Color::Green.b, 255), "Successfully loaded patterns.");
        }
        else
        {
            ImGui::TextColored(ImVec4(sf::Color::Red.r, sf::Color::Red.g, sf::Color::Red.b, 255), "Failed to load patterns.");
        }

        if (ImGui::IsMouseClicked(ImGuiMouseButton_::ImGuiMouseButton_Left))
        {
            m_isOpen_LoadFileResult = false;
        }

        ImGui::EndPopup();
    }

    /*
    if (ImGui::BeginPopupModal("Failed to load the file"))
    {

        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal("Failed to load file"))
    {

        ImGui::EndPopup();
    }
    */
}

void OCR_App::configureGUI()
{
    m_menuWindowWidthInPixels = (m_menuWindowSizePercentage / 100) * m_window.getSize().x;

    sf::Vector2f contentSize(m_window.getSize().x - m_menuWindowWidthInPixels, m_window.getSize().y);

    m_canvasRect.setSize(sf::Vector2f(m_canvasImage.getSize().x, m_canvasImage.getSize().y));
    m_canvasRect.setPosition(sf::Vector2f(contentSize.x - (contentSize.x / 2) - (m_canvasRect.getSize().x / 2), contentSize.y - (contentSize.y / 2) - (m_canvasRect.getSize().y / 2)));

    m_canvasRect.setTexture(&m_canvasTexture, true);
}

void OCR_App::clearCanvas()
{
    m_canvasImage.create(400, 400, m_canvasBackgroundColor);
    m_canvasTexture.loadFromImage(m_canvasImage);
    m_boundingBoxVertexArray.clear();
}

void OCR_App::handleDrawing()
{
    if (m_isOpen_LoadDefaultPatterns || m_isOpen_RecognitionResultModal | m_isOpen_LoadFileResult)
    {
        return;
    }

    if (!sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
    {
        m_lastMousePositionInImageInitialized = false;
        return;
    }

    sf::Vector2i mousePos = sf::Mouse::getPosition(m_window);
    sf::Vector2f canvasPosition = m_canvasRect.getPosition();

    if (mousePos.x < canvasPosition.x || mousePos.x > canvasPosition.x + m_canvasRect.getSize().x ||
        mousePos.y < canvasPosition.y || mousePos.y > canvasPosition.y + m_canvasRect.getSize().y)
    {
        return;
    }


    sf::Vector2f mousePosInImage(mousePos.x - canvasPosition.x, mousePos.y - canvasPosition.y);


    if (mousePosInImage.x <= 0 || mousePosInImage.x >= m_canvasImage.getSize().x ||
        mousePosInImage.y <= 0 || mousePosInImage.y >= m_canvasImage.getSize().y)
    {
        return;
    }


    if (m_canvasImage.getPixel(mousePosInImage.x, mousePosInImage.y) == m_drawPixelsColor)
    {
        return;
    }



    if (!m_lastMousePositionInImageInitialized)
    {
        m_canvasImage.setPixel(mousePosInImage.x, mousePosInImage.y, m_drawPixelsColor);
    }
    else
    {
        drawLine(m_canvasImage, sf::Vector2f(m_lastMousePositionInImage.x, m_lastMousePositionInImage.y), sf::Vector2f(mousePosInImage.x, mousePosInImage.y));
    }
    

    m_canvasTexture.update(m_canvasImage);


    m_lastMousePositionInImage = mousePosInImage;
    m_lastMousePositionInImageInitialized = true;
}

void OCR_App::drawLine(sf::Image& targetImage, sf::Vector2f pointA, sf::Vector2f pointB)
{
    //modified code from http://rosettacode.org/wiki/Bitmap/Bresenham%27s_line_algorithm#C.2B.2B
    const bool steep = (fabs(pointB.y - pointA.y) > fabs(pointB.x - pointA.x));
    if (steep)
    {
        std::swap(pointA.x, pointA.y);
        std::swap(pointB.x, pointB.y);
    }

    if (pointA.x > pointB.x)
    {
        std::swap(pointA.x, pointB.x);
        std::swap(pointA.y, pointB.y);
    }

    const float dx = pointB.x - pointA.x;
    const float dy = fabs(pointB.y - pointA.y);

    float error = dx / 2.0f;
    const int ystep = (pointA.y < pointB.y) ? 1 : -1;
    int y = (int)pointA.y;

    const int maxX = (int)pointB.x;

    for (int x = (int)pointA.x; x <= maxX; x++)
    {
        if (steep)
        {
            targetImage.setPixel(y, x, m_drawPixelsColor);
        }
        else
        {
            targetImage.setPixel(x, y, m_drawPixelsColor);
        }

        error -= dy;
        if (error < 0)
        {
            y += ystep;
            error += dx;
        }
    }
}

uint64_t OCR_App::generateCharacterBinaryImage()
{
    uint64_t value = 0;

    //this value must be 8
    uint8_t m_binaryImageResolution = 8;

    sf::Rect<uint32_t> characterRect = getRectOfCharacter();

    float cellWidth = (float)(characterRect.width + 1) / m_binaryImageResolution;
    float cellHeight = (float)(characterRect.height + 1) / m_binaryImageResolution;

    std::bitset<64> bitset;

    for (size_t cellY = 0; cellY < m_binaryImageResolution; cellY++)
    {
        for (size_t cellX = 0; cellX < m_binaryImageResolution; cellX++)
        {
            if (!getBinaryImageCellValue(m_canvasImage, sf::Rect<uint32_t>(characterRect.left + (cellWidth * cellX), characterRect.top + (cellHeight * cellY), cellWidth, cellHeight)))
            {
                //std::cout << '0';
                bitset.set(cellY + (cellX * m_binaryImageResolution), false);
                continue;
            }

            bitset.set(cellY + (cellX * m_binaryImageResolution), true);
            //std::cout << '1';
        }

        //std::cout << std::endl;
    }

    return bitset.to_ullong();
}

bool OCR_App::getBinaryImageCellValue(const sf::Image& image, const sf::Rect<uint32_t>& cell)
{
    
    for (size_t x = 0; x < cell.width; x++)
    {
        for (size_t y = 0; y < cell.height; y++)
        {
            if (image.getPixel(cell.left + x, cell.top + y) == m_drawPixelsColor)
            {
                return true;
            }
        }
    }

    return false;
}

sf::Rect<uint32_t> OCR_App::getRectOfCharacter() const
{
    sf::Vector2u min;
    sf::Vector2u max;

    sf::Vector2u canvasSize = m_canvasImage.getSize();

    bool foundFirstDrawnPixel = false;

    for (size_t y = 0; y < canvasSize.y; y++)
    {
        for (size_t x = 0; x < canvasSize.x; x++)
        {
            if (m_canvasImage.getPixel(x, y) != m_drawPixelsColor)
            {
                continue;
            }

            if (!foundFirstDrawnPixel)
            {
                min.x = x;
                min.y = y;

                max.x = x;
                max.y = y;

                foundFirstDrawnPixel = true;
            }

            min.x = std::min(min.x, x);
            min.y = std::min(min.y, y);

            max.x = std::max(max.x, x);
            max.y = std::max(max.y, y);
        }
    }

    //std::cout << "min x: " << min.x << " | min y: " << min.y << " | max x: " << max.x << " | max y: " << max.y << std::endl;

    return sf::Rect<uint32_t>(min.x, min.y, 1 + max.x - min.x, 1 + max.y - min.y);
}

void OCR_App::train(uint64_t binarydata)
{
    /*
    std::bitset<64> bitset = binarydata;

    std::vector<boost::numeric::ublas::vector<double>> trainingData;

    trainingData.resize(1);
    trainingData[0].resize(bitset.size());

    for (size_t i = 0; i < bitset.size(); i++)
    {
        trainingData[0][i] = bitset[i];
    }


    //Initialize SOM object class
    neuralnetworks::SelfOrganizingMaps obj(1, m_binaryImageResolution.x, m_binaryImageResolution.y);
    puts("\nSOM Object initialization...done\n");

    //Write to the SOM class object
    obj.trainingData.swap(trainingData);
    trainingData.clear();
    puts("Train data written to SOM object");

    //Weight Initialization
    obj.weightsInitialization(0.1, 0.1);
    puts("SOM Weights initialization...done");

    //SOM TRAINING
    //Use size/100 for bootstrap
    obj.somTraining(100, 0.1);
    puts("SOM Training...done");
    */
}

char OCR_App::recognize(uint64_t binaryImage)
{
    uint16_t smallestDifference = 64;
    char mostAccurateCharacter = 0;

    for (auto characterIterator = m_charactersPatterns.begin(); characterIterator != m_charactersPatterns.end(); characterIterator++)
    {
        for (auto patternIterator = characterIterator->second.begin(); patternIterator != characterIterator->second.end(); patternIterator++)
        {
            uint16_t difference = countDifferentBits(binaryImage, *patternIterator);
            if (countDifferentBits(binaryImage, *patternIterator) > m_bitTolerance)
            {
                continue;
            }

            if (difference < smallestDifference)
            {
                smallestDifference = difference;
                mostAccurateCharacter = characterIterator->first;
            }
        }
    }

    return mostAccurateCharacter;
}

uint16_t OCR_App::countDifferentBits(uint64_t a, uint64_t b) const
{
    std::bitset<64> bits = a ^ b;
    return bits.count();
}

bool OCR_App::loadPatterns(const char* path)
{
    m_charactersPatterns.clear();

    std::fstream file;

    file.open(path, std::ios::in | std::ios::ate | std::ios::binary);

    if (!file.is_open() || !file.good())
    {
        return false;
    }

    size_t fileSize = file.tellg();
    file.seekg(0);

    char character = 0;
    uint32_t patternCount = 0;
    size_t filePos = 0;


    while (filePos < fileSize)
    {
        file.read(&character, sizeof(char));
        file.read((char*)&patternCount, sizeof(uint32_t));

        filePos += sizeof(char);
        filePos += sizeof(uint32_t);


        auto& characterPatterns = m_charactersPatterns[character];

        characterPatterns.resize(patternCount);

        for (size_t patternIterator = 0; patternIterator < patternCount; patternIterator++)
        {
            uint64_t pattern = 0;
            file.read((char*)&characterPatterns[patternIterator], sizeof(uint64_t));
            filePos += sizeof(uint64_t);
        }
    }

    file.close();


    return true;
}

bool OCR_App::savePatterns(const char* path) const
{
    std::fstream file;

    file.open(path, std::ios::out | std::ios::beg | std::ios::binary | std::ios::trunc);

    if (!file.is_open() || !file.good())
    {
        return false;
    }


    for (auto characterIterator = m_charactersPatterns.begin(); characterIterator != m_charactersPatterns.end(); characterIterator++)
    {
        uint32_t patternCount = characterIterator->second.size();

        file.write(&characterIterator->first, sizeof(char));
        file.write((const char*)&patternCount, sizeof(uint32_t));

        for (auto patternIterator = characterIterator->second.begin(); patternIterator != characterIterator->second.end(); patternIterator++)
        {
            file.write((const char*)&*patternIterator, sizeof(uint64_t));
        }
    }

    file.close();

    return true;
}

void OCR_App::update()
{
    handleEvents();

    m_window.clear();
    m_window.draw(m_canvasRect);

    handleDrawing();

    if (m_boundingBoxVertexArray.getVertexCount() > 0)
    {
        m_window.draw(&m_boundingBoxVertexArray[0], m_boundingBoxVertexArray.getVertexCount(), sf::PrimitiveType::LinesStrip);
    }


    ImGui::SFML::Update(m_window, m_deltaClock.restart());

    renderMainWindowBar();
    renderMenuWindow();
    renderModals();

    
    ImGui::SFML::Render(m_window);

    m_window.display();
}

bool OCR_App::isOpen() const
{
	return m_window.isOpen();
}
