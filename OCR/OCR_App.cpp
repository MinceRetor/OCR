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
    m_patternsWindowSelectedCharacter(0),
    m_bitTolerance(15),
    m_binaryImagePreviewButtonSize(100, 100),
    m_defaultPatternsFilePath("defaultPatterns.cpf"),
    m_isOpen_RecognitionResultModal(false),
    m_isOpen_LoadDefaultPatterns(true),
    m_isOpen_PatternsWindow(false),
    m_isOpen_StyleSettingsWindow(false),
    m_loadFileErrorMsg(nullptr)
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
        if (ImGui::BeginMenu("File"))
        {
            
            if (ImGui::Selectable("Load Patterns"))
            {
                NFD::UniquePath path;
                if (NFD::OpenDialog(path, characterPatternsFileFilters, characterPatternsFileFiltersCount) == NFD_OKAY)
                {
                    m_charactersPatterns.clear();
                    m_fileLoadedResult = loadPatterns(path.get(), &m_loadFileErrorMsg);
                    m_isOpen_LoadFileResult = true;
                }
            }

            if (ImGui::Selectable("Load Patterns Additive"))
            {
                NFD::UniquePath path;
                if (NFD::OpenDialog(path, characterPatternsFileFilters, characterPatternsFileFiltersCount) == NFD_OKAY)
                {
                    m_fileLoadedResult = loadPatterns(path.get(), &m_loadFileErrorMsg);
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

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Selectable("Clear Patterns"))
            {
                m_charactersPatterns.clear();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::Selectable("Patterns"))
            {
                m_isOpen_PatternsWindow = true;
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Selectable("Style"))
            {
                m_isOpen_StyleSettingsWindow = true;
            }
            
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void OCR_App::renderMenuWindow()
{
    ImGui::SetNextWindowSize(ImVec2(m_menuWindowWidthInPixels, m_window.getSize().y - 20));
    ImGui::SetNextWindowPos(ImVec2(m_window.getSize().x - m_menuWindowWidthInPixels, 20));
    ImGui::Begin("##MenuWindow", nullptr, ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);


    std::string characterLabel;
    characterLabel.resize(1);
    characterLabel[0] = m_character;

    ImGui::Spacing();

    binaryImagePreview(generateCharacterBinaryImage(), ImVec2(100, 100));

    ImGui::Spacing();

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Spacing();

    if (ImGui::Button("Recognize"))
    {
        m_recognizedCharacter = recognize(generateCharacterBinaryImage());

        m_isOpen_RecognitionResultModal = true;
    }

    ImGui::SameLine();

    ImGui::SetNextItemWidth(ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x);
    if (ImGui::Button("Clear Canvas"))
    {
        clearCanvas();
    }

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Spacing();

    ImGui::SetNextItemWidth(ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x);
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

        for (char character = 'A'; character <= 'Z'; character++)
        {
            characterLabel[0] = character;

            if (ImGui::Selectable(characterLabel.c_str()))
            {
                m_character = character;
            }
        }

        ImGui::EndCombo();
    }

    ImGui::Spacing();

    if (ImGui::Button("Add Pattern"))
    {
        m_charactersPatterns[m_character].push_back(generateCharacterBinaryImage());
    }

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Spacing();

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

    

    ImGui::End();
}

void OCR_App::renderPatternsWindow()
{
    if (!m_isOpen_PatternsWindow)
    {
        return;
    }
    ImGui::Begin("Patterns##PatternsWindow", &m_isOpen_PatternsWindow, ImGuiWindowFlags_::ImGuiWindowFlags_MenuBar);
    

    if (ImGui::BeginMenuBar())
    {
        ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_Text, ImVec4(255, 0, 0, 255));
        if (ImGui::MenuItem("Delete Selected##PatternsWindow"))
        {
            deleteSelectedPatterns();
        }
        ImGui::PopStyleColor();
        ImGui::EndMenuBar();
    }
    

    ImVec2 windowContentRegionMin = ImGui::GetWindowContentRegionMin();
    ImVec2 windowContentRegionMax = ImGui::GetWindowContentRegionMax();
    ImVec2 windowContentSize(windowContentRegionMax.x - windowContentRegionMin.x, windowContentRegionMax.y - windowContentRegionMin.y);

    ImGui::BeginChild("##PatternsWindowCharactersList", ImVec2(windowContentSize.x * 0.1, windowContentSize.y), true);
    char characterString[2];
    characterString[1] = 0;

    for (characterString[0] = '0'; characterString[0] <= '9'; characterString[0]++)
    {
        if (ImGui::Selectable(characterString))
        {
            m_patternsWindowSelectedCharacter = characterString[0];
            m_selectedPatternsToRemove.clear();
        }
    }

    for (characterString[0] = 'A'; characterString[0] <= 'Z'; characterString[0]++)
    {
        if (ImGui::Selectable(characterString))
        {
            m_patternsWindowSelectedCharacter = characterString[0];
            m_selectedPatternsToRemove.clear();
        }
    }

    ImGui::EndChild();

    ImGui::SameLine();

    float patternsChildWindowWidth = windowContentSize.x * 0.85;


    ImGui::BeginChild("##PatternsWindowCharacter", ImVec2(patternsChildWindowWidth, windowContentSize.y), true);
    if (m_patternsWindowSelectedCharacter)
    {
        auto& characterPatterns = m_charactersPatterns[m_patternsWindowSelectedCharacter];

        ImVec2 patternsChildWindowRegionMax = ImGui::GetWindowContentRegionMax();

        float cursorPos = 0;

        for (size_t i = 0; i != characterPatterns.size(); i++)
        {
            if (cursorPos + m_binaryImagePreviewButtonSize.x > patternsChildWindowWidth)
            {
                cursorPos = 0;
            }
            else if (i > 0)
            {
                ImGui::SameLine();
            }

            std::string id = "##binaryImageButton";
            id += m_patternsWindowSelectedCharacter;
            id += std::to_string(i);


            bool selected = isSelectedPatternsToRemove(i);

            if (selected)
            {
                ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_::ImGuiCol_ButtonActive));
            }
            if (binaryImageButton(id.c_str(), characterPatterns[i], m_binaryImagePreviewButtonSize))
            {
                selectOrUnselectPatternToRemove(i);
            }
            if (selected)
            {
                ImGui::PopStyleColor();
            }

            cursorPos += m_binaryImagePreviewButtonSize.x;
        }

    }

    ImGui::EndChild();

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
                loadedDefaultPatternsStatus = loadPatterns(m_defaultPatternsFilePath, &m_loadFileErrorMsg);
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

                if (m_loadFileErrorMsg != nullptr)
                {
                    ImGui::TextColored(ImVec4(sf::Color::Red.r, sf::Color::Red.g, sf::Color::Red.b, 255), m_loadFileErrorMsg);
                }
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

            if (m_loadFileErrorMsg != nullptr)
            {
                ImGui::TextColored(ImVec4(sf::Color::Red.r, sf::Color::Red.g, sf::Color::Red.b, 255), m_loadFileErrorMsg);
            }
        }

        if (ImGui::IsMouseClicked(ImGuiMouseButton_::ImGuiMouseButton_Left))
        {
            m_isOpen_LoadFileResult = false;
        }

        ImGui::EndPopup();
    }
}

void OCR_App::binaryImagePreview(const binaryImageType& image, const ImVec2& size)
{
    const ImVec2 cellSize(size.x / BINARY_IMAGE_WIDTH, size.y / BINARY_IMAGE_HEIGHT);

    const ImVec2 startCursorPos = ImGui::GetCursorScreenPos();

    ImGui::ItemSize(ImRect(startCursorPos, ImVec2(startCursorPos.x + size.x, startCursorPos.y + size.y)));

    ImVec2 cursorPos = startCursorPos;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    for (size_t cellY = 0; cellY < BINARY_IMAGE_HEIGHT; cellY++)
    {
        for (size_t cellX = 0; cellX < BINARY_IMAGE_WIDTH; cellX++)
        {
            const ImU32* cellColor = nullptr;

            if (image[cellY + (cellX * BINARY_IMAGE_HEIGHT)])
            {
                cellColor = &binaryImageViewColor;
            }
            else
            {
                cellColor = &binaryImageViewBackgroundColor;
            }

            draw_list->AddRectFilled(ImVec2(cursorPos.x, cursorPos.y), ImVec2(cursorPos.x + cellSize.x, cursorPos.y + cellSize.y), *cellColor);

            cursorPos.x += cellSize.x;
        }

        cursorPos.x = startCursorPos.x;
        cursorPos.y += cellSize.y;
    }
}

bool OCR_App::binaryImageButton(const char* id, const binaryImageType& image, const ImVec2& size)
{
    ImVec2 imageSize = size;
    imageSize.x *= 0.9;
    imageSize.y *= 0.9;


    ImVec2 cursorStartPos = ImGui::GetCursorPos();
    ImVec2 imageStartCursorPos = cursorStartPos;
    imageStartCursorPos.x += (size.x - imageSize.x) / 2;
    imageStartCursorPos.y += (size.y - imageSize.y) / 2;


    bool buttonResult = ImGui::Button(id, size);

    ImVec2 buttonEndCursorPos = ImGui::GetCursorPos();


    ImGui::SetCursorPos(imageStartCursorPos);

    const ImVec2 cellSize(imageSize.x / BINARY_IMAGE_WIDTH, imageSize.y / BINARY_IMAGE_HEIGHT);

    const ImVec2 startCursorPos = ImGui::GetCursorScreenPos();

    ImVec2 cursorPos = startCursorPos;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    for (size_t cellY = 0; cellY < BINARY_IMAGE_HEIGHT; cellY++)
    {
        for (size_t cellX = 0; cellX < BINARY_IMAGE_WIDTH; cellX++)
        {
            const ImU32* cellColor = nullptr;

            if (image[cellY + (cellX * BINARY_IMAGE_HEIGHT)])
            {
                cellColor = &binaryImageViewColor;
            }
            else
            {
                cellColor = &binaryImageViewBackgroundColor;
            }

            draw_list->AddRectFilled(ImVec2(cursorPos.x, cursorPos.y), ImVec2(cursorPos.x + cellSize.x, cursorPos.y + cellSize.y), *cellColor);

            cursorPos.x += cellSize.x;
        }

        cursorPos.x = startCursorPos.x;
        cursorPos.y += cellSize.y;
    }


    ImGui::SetCursorPos(buttonEndCursorPos);

    return buttonResult;
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
    if (m_isOpen_LoadDefaultPatterns || m_isOpen_RecognitionResultModal || m_isOpen_LoadFileResult || m_isOpen_StyleSettingsWindow || m_isOpen_PatternsWindow)
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

binaryImageType OCR_App::generateCharacterBinaryImage()
{
    sf::Rect<uint32_t> characterRect = getRectOfCharacter();

    float cellWidth = (float)(characterRect.width + 1) / BINARY_IMAGE_WIDTH;
    float cellHeight = (float)(characterRect.height + 1) / BINARY_IMAGE_HEIGHT;

    std::bitset<BINARY_IMAGE_WIDTH* BINARY_IMAGE_HEIGHT> bitset;

    for (size_t cellY = 0; cellY < BINARY_IMAGE_HEIGHT; cellY++)
    {
        for (size_t cellX = 0; cellX < BINARY_IMAGE_WIDTH; cellX++)
        {
            if (!getBinaryImageCellValue(m_canvasImage, sf::Rect<uint32_t>(characterRect.left + (cellWidth * cellX), characterRect.top + (cellHeight * cellY), cellWidth, cellHeight)))
            {
                bitset.set(cellY + (cellX * BINARY_IMAGE_WIDTH), false);
                continue;
            }

            bitset.set(cellY + (cellX * BINARY_IMAGE_HEIGHT), true);
        }
    }

    return bitset;
}

bool OCR_App::getBinaryImageCellValue(const sf::Image& image, const sf::Rect<uint32_t>& cell)
{
    sf::Vector2u imageSize = image.getSize();

    if (cell.left > imageSize.x || cell.top > imageSize.y)
    {
        return false;
    }

    if (cell.left + cell.width > imageSize.x || cell.top + cell.height > imageSize.y)
    {
        return false;
    }

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

bool OCR_App::isSelectedPatternsToRemove(uint32_t patternIndex) const
{
    for (size_t i = 0; i < m_selectedPatternsToRemove.size(); i++)
    {
        if (m_selectedPatternsToRemove[i] == patternIndex)
        {
            return true;
        }
    }
    return false;
}

void OCR_App::selectOrUnselectPatternToRemove(uint32_t patternIndex)
{
    for (size_t i = 0; i < m_selectedPatternsToRemove.size(); i++)
    {
        if (m_selectedPatternsToRemove[i] == patternIndex)
        {
            m_selectedPatternsToRemove.erase(m_selectedPatternsToRemove.begin() + i);
            return;
        }
    }
    m_selectedPatternsToRemove.push_back(patternIndex);
}

void OCR_App::deleteSelectedPatterns()
{

    if (m_patternsWindowSelectedCharacter == 0)
    {
        return;
    }

    auto& patterns = m_charactersPatterns[m_patternsWindowSelectedCharacter];


    for (size_t i = 0; i < m_selectedPatternsToRemove.size(); i++)
    {
        if (m_selectedPatternsToRemove[i] < patterns.size())
        {
            patterns.erase(patterns.begin() + m_selectedPatternsToRemove[i]);
        }
    }
    m_selectedPatternsToRemove.clear();
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

char OCR_App::recognize(const binaryImageType& binaryImage)
{
    uint16_t smallestDifference = 64;
    char mostAccurateCharacter = 0;

    for (auto characterIterator = m_charactersPatterns.begin(); characterIterator != m_charactersPatterns.end(); characterIterator++)
    {
        for (auto patternIterator = characterIterator->second.begin(); patternIterator != characterIterator->second.end(); patternIterator++)
        {
            uint16_t difference = countInconsistentBits(binaryImage, *patternIterator);
            if (countInconsistentBits(binaryImage, *patternIterator) > m_bitTolerance)
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

uint32_t OCR_App::countInconsistentBits(const binaryImageType& a, const binaryImageType& b) const
{
    std::bitset<BINARY_IMAGE_WIDTH* BINARY_IMAGE_HEIGHT> bits = a ^ b;
    return bits.count();
}

bool OCR_App::loadPatterns(const char* path, const char** errorMsg)
{
    std::fstream file;

    file.open(path, std::ios::in | std::ios::ate | std::ios::binary);

    if (!file.is_open() || !file.good())
    {
        if (errorMsg != nullptr)
        {
            *errorMsg = "Failed to open file.";
        }
        return false;
    }

    size_t fileSize = file.tellg();
    file.seekg(0);

    char character = 0;
    uint32_t patternCount = 0;
    size_t filePos = 0;


    if (filePos + 3 + (2 * sizeof(uint32_t)) >= fileSize)
    {
        if (errorMsg != nullptr)
        {
            *errorMsg = "Failed to read file header.";
        }
        return false;
    }

    std::vector<char> fileType(3);

    file.read(fileType.data(), 3);
    filePos += 3;


    if (std::string(fileType.data()) == "CPF")
    {
        if (errorMsg != nullptr)
        {
            *errorMsg = "File is not a CPF file.";
        }
        return false;
    }


    uint32_t binaryImageWidth = 0;
    uint32_t binaryImageHeight = 0;

    file.read((char*)&binaryImageWidth, sizeof(uint32_t));
    file.read((char*)&binaryImageHeight, sizeof(uint32_t));
    filePos += sizeof(uint32_t);
    filePos += sizeof(uint32_t);


    if (binaryImageWidth != BINARY_IMAGE_WIDTH || binaryImageHeight != BINARY_IMAGE_HEIGHT)
    {
        if (errorMsg != nullptr)
        {
            *errorMsg = "Patterns saved in file has different resolution than this program version use.";
        }
        return false;
    }


    while (filePos < fileSize)
    {
        file.read(&character, sizeof(char));
        file.read((char*)&patternCount, sizeof(uint32_t));

        filePos += sizeof(char);
        filePos += sizeof(uint32_t);


        auto& characterPatterns = m_charactersPatterns[character];

        for (size_t patternIterator = 0; patternIterator < patternCount; patternIterator++)
        {
            std::string data;
            data.resize(BINARY_IMAGE_WIDTH * BINARY_IMAGE_HEIGHT);


            file.read(&data[0], data.size());
            filePos += data.size();

            characterPatterns.push_back(binaryImageType(data));
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

    static const uint32_t binaryImageWidth = BINARY_IMAGE_WIDTH;
    static const uint32_t binaryImageHeight = BINARY_IMAGE_HEIGHT;

    file.write("CPF", 3);
    file.write((const char*)&binaryImageWidth, sizeof(uint32_t));
    file.write((const char*)&binaryImageHeight, sizeof(uint32_t));


    for (auto characterIterator = m_charactersPatterns.begin(); characterIterator != m_charactersPatterns.end(); characterIterator++)
    {
        uint32_t patternCount = characterIterator->second.size();

        file.write(&characterIterator->first, sizeof(char));
        file.write((const char*)&patternCount, sizeof(uint32_t));

        for (auto patternIterator = characterIterator->second.begin(); patternIterator != characterIterator->second.end(); patternIterator++)
        {
            std::string data = patternIterator->to_string();
            file.write(data.data(), data.size());
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
    renderPatternsWindow();
    renderStyleSettingsWindow();
    renderModals();

    ImGui::SFML::Render(m_window);

    m_window.display();
}

bool OCR_App::isOpen() const
{
	return m_window.isOpen();
}

void OCR_App::renderStyleSettingsWindow()
{
    if (!m_isOpen_StyleSettingsWindow)
    {
        return;
    }

    if (ImGui::Begin("Style Settings", &m_isOpen_StyleSettingsWindow))
    {
        ImGuiStyle& style = ImGui::GetStyle();

        //if (ImGui::CollapsingHeader("Colors"))
        {
            ImGui::ColorEdit4("Border Color", (float*)&style.Colors[ImGuiCol_::ImGuiCol_Border]);
            ImGui::ColorEdit4("Border Shadow", (float*)&style.Colors[ImGuiCol_::ImGuiCol_BorderShadow]);
            ImGui::ColorEdit4("Button", (float*)&style.Colors[ImGuiCol_::ImGuiCol_Button]);
            ImGui::ColorEdit4("Button Active", (float*)&style.Colors[ImGuiCol_::ImGuiCol_ButtonActive]);
            ImGui::ColorEdit4("Button Hovered", (float*)&style.Colors[ImGuiCol_::ImGuiCol_ButtonHovered]);
            ImGui::ColorEdit4("Check Mark", (float*)&style.Colors[ImGuiCol_::ImGuiCol_CheckMark]);
            ImGui::ColorEdit4("Child Background", (float*)&style.Colors[ImGuiCol_::ImGuiCol_ChildBg]);
            ImGui::ColorEdit4("Dag And Drop Target", (float*)&style.Colors[ImGuiCol_::ImGuiCol_DragDropTarget]);
            ImGui::ColorEdit4("Frame Background", (float*)&style.Colors[ImGuiCol_::ImGuiCol_FrameBg]);
            ImGui::ColorEdit4("Frame Active Background", (float*)&style.Colors[ImGuiCol_::ImGuiCol_FrameBgActive]);
            ImGui::ColorEdit4("Frame Hovered Background", (float*)&style.Colors[ImGuiCol_::ImGuiCol_FrameBgHovered]);
            ImGui::ColorEdit4("PrimaryHeader", (float*)&style.Colors[ImGuiCol_::ImGuiCol_Header]);
            ImGui::ColorEdit4("PrimaryHeader Active", (float*)&style.Colors[ImGuiCol_::ImGuiCol_HeaderActive]);
            ImGui::ColorEdit4("PrimaryHeader Hovered", (float*)&style.Colors[ImGuiCol_::ImGuiCol_HeaderHovered]);
            ImGui::ColorEdit4("Menu Bar Background", (float*)&style.Colors[ImGuiCol_::ImGuiCol_MenuBarBg]);
            ImGui::ColorEdit4("Modal Window Dim Background", (float*)&style.Colors[ImGuiCol_::ImGuiCol_ModalWindowDimBg]);
            ImGui::ColorEdit4("Nav Highlight", (float*)&style.Colors[ImGuiCol_::ImGuiCol_NavHighlight]);
            ImGui::ColorEdit4("Nav Windowing Highlight", (float*)&style.Colors[ImGuiCol_::ImGuiCol_NavWindowingHighlight]);
            ImGui::ColorEdit4("Plot Histogram Hovered", (float*)&style.Colors[ImGuiCol_::ImGuiCol_PlotHistogramHovered]);
            ImGui::ColorEdit4("Plot Lines", (float*)&style.Colors[ImGuiCol_::ImGuiCol_PlotLines]);
            ImGui::ColorEdit4("Popup Background", (float*)&style.Colors[ImGuiCol_::ImGuiCol_PopupBg]);
            ImGui::ColorEdit4("Resize Grip", (float*)&style.Colors[ImGuiCol_::ImGuiCol_ResizeGrip]);
            ImGui::ColorEdit4("Resize Grip Active", (float*)&style.Colors[ImGuiCol_::ImGuiCol_ResizeGripActive]);
            ImGui::ColorEdit4("Resize Grip Hovered", (float*)&style.Colors[ImGuiCol_::ImGuiCol_ResizeGripHovered]);
            ImGui::ColorEdit4("Scrollbar Background", (float*)&style.Colors[ImGuiCol_::ImGuiCol_ScrollbarBg]);
            ImGui::ColorEdit4("Scrollbar Grab", (float*)&style.Colors[ImGuiCol_::ImGuiCol_ScrollbarGrab]);
            ImGui::ColorEdit4("Scrollbar Grab Active", (float*)&style.Colors[ImGuiCol_::ImGuiCol_ScrollbarGrabActive]);
            ImGui::ColorEdit4("Scrollbar Grab Hovered", (float*)&style.Colors[ImGuiCol_::ImGuiCol_ScrollbarGrabHovered]);
            ImGui::ColorEdit4("Separator", (float*)&style.Colors[ImGuiCol_::ImGuiCol_Separator]);
            ImGui::ColorEdit4("Separator Active", (float*)&style.Colors[ImGuiCol_::ImGuiCol_SeparatorActive]);
            ImGui::ColorEdit4("Separator Hovered", (float*)&style.Colors[ImGuiCol_::ImGuiCol_SeparatorHovered]);
            ImGui::ColorEdit4("Slider Grab", (float*)&style.Colors[ImGuiCol_::ImGuiCol_SliderGrab]);
            ImGui::ColorEdit4("Slider Grab Active", (float*)&style.Colors[ImGuiCol_::ImGuiCol_SliderGrabActive]);
            ImGui::ColorEdit4("Tab", (float*)&style.Colors[ImGuiCol_::ImGuiCol_Tab]);
            ImGui::ColorEdit4("Tab Active", (float*)&style.Colors[ImGuiCol_::ImGuiCol_TabActive]);
            ImGui::ColorEdit4("Tab Hovered", (float*)&style.Colors[ImGuiCol_::ImGuiCol_TabHovered]);
            ImGui::ColorEdit4("Tab Unfocused", (float*)&style.Colors[ImGuiCol_::ImGuiCol_TabUnfocused]);
            ImGui::ColorEdit4("Tab Unfocused Active", (float*)&style.Colors[ImGuiCol_::ImGuiCol_TabUnfocusedActive]);
            ImGui::ColorEdit4("Text", (float*)&style.Colors[ImGuiCol_::ImGuiCol_Text]);
            ImGui::ColorEdit4("Text Disabled", (float*)&style.Colors[ImGuiCol_::ImGuiCol_TextDisabled]);
            ImGui::ColorEdit4("Text Selected Background", (float*)&style.Colors[ImGuiCol_::ImGuiCol_TextSelectedBg]);
            ImGui::ColorEdit4("Title Background", (float*)&style.Colors[ImGuiCol_::ImGuiCol_TitleBg]);
            ImGui::ColorEdit4("Title Background Active", (float*)&style.Colors[ImGuiCol_::ImGuiCol_TitleBgActive]);
            ImGui::ColorEdit4("Title Background Collapsed", (float*)&style.Colors[ImGuiCol_::ImGuiCol_TitleBgCollapsed]);
            ImGui::ColorEdit4("WindowB Background", (float*)&style.Colors[ImGuiCol_::ImGuiCol_WindowBg]);

        }


        ImGui::End();
    }
}


ImU32 OCR_App::binaryImageViewColor(ImColor(0, 0, 0, 255));
ImU32 OCR_App::binaryImageViewBackgroundColor(ImColor(255, 255, 255, 255));