#include <SFML/Graphics.hpp>
#include <utility>
#include <unordered_map>
#include <memory>
#include <iostream>
#include <vector>
#include <typeindex> 
#include <random>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600; 

class Entités {
private:
    int id;

    static int IdGenerateur() {
        static int currentid = 0;
        return currentid++;
    }

    std::unordered_map<std::type_index, std::shared_ptr<void>> components;

public:
    Entités() : id(IdGenerateur()) {}

    int GetID() const {
        return id;
    }

    template <typename T>
    void addComponent(T component) {
        components[std::type_index(typeid(T))] = std::make_shared<T>(component);
    }

    template <typename T>
    T& getComponent() {
        return *std::static_pointer_cast<T>(components.at(std::type_index(typeid(T))));
    }

    template <typename T>
    bool hasComponent() const {
        return components.find(std::type_index(typeid(T))) != components.end();
    }
};

class Position2D {
private:
    float x, y;
    float hauteur, largeur;

public:
    std::pair<float, float> GetPosition() const {
        return { x, y };
    }

    std::pair<float, float> GetSize() const {
        return { hauteur, largeur };
    }
    
    void SetPosition(float newX, float newY) {
        x = newX;
        y = newY;
    }

    Position2D(float x = 0, float y = 0, float hauteur = 0, float largeur = 0)
        : x(x), y(y), hauteur(hauteur), largeur(largeur) {}
};

class Velocity {
private:
    float vx, vy;

public:
    std::pair<float, float> GetSpeed() const {
        return { vx, vy };
    }

    Velocity(float vx = 0.0f, float vy = 0.0f) : vx(vx), vy(vy) {}
};

class StyleForme {
public:
    enum class TypeFormes { Cercle, Rectangle }; // Correction de l'énum

    StyleForme(TypeFormes forme, sf::Color color = sf::Color::White)
        : forme(forme), color(color) {}

    TypeFormes forme;
    sf::Color color;
};

class Comportement {
public:
    enum class TypesComportement { Balle, Brique, Pad };
    Comportement(TypesComportement type) : type(type) {}

    TypesComportement GetType() const {
        return type;
    }

private:
    TypesComportement type;
};

class EntityManager {
private:
    std::vector<std::shared_ptr<Entités>> entities;
    int niveau = 1;                // Niveau de difficulté
    float vitesseBalle = 0.1f;     // Vitesse de base de la balle
    int score = 0;
public:
    void addEntity(const std::shared_ptr<Entités>& entity) {
        entities.push_back(entity);
    }

    void removeEntity(const std::shared_ptr<Entités>& entity) {
        entities.erase(std::remove(entities.begin(), entities.end(), entity), entities.end());
    }

    bool isLevelComplete(EntityManager& manager) {
        for (const auto& entity : manager.getEntities()) {
            if (entity->hasComponent<Comportement>() && entity->getComponent<Comportement>().GetType() == Comportement::TypesComportement::Brique) {
                return false;
            }
        }
        return true;
    }

    void updateEntities(float screenWidth, float screenHeight) {

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(0.1f, 0.2f);

        for (auto it = entities.begin(); it != entities.end(); ) {
            auto& entity = *it;
            if (entity->hasComponent<Position2D>() && entity->hasComponent<Velocity>()) {
                auto& pos = entity->getComponent<Position2D>();
                auto& vel = entity->getComponent<Velocity>();

                float newPosX = pos.GetPosition().first + vel.GetSpeed().first;
                float newPosY = pos.GetPosition().second + vel.GetSpeed().second;

                // Vérifier les collisions avec les bords de l'écran et inverser la vitesse si nécessaire
                if (newPosX <= 0 || newPosX >= screenWidth - pos.GetSize().first) {
                    vel = Velocity(-vel.GetSpeed().first, vel.GetSpeed().second);  // Inverser la vitesse en X
                }
                if (newPosY <= 0) { // Collision avec le haut
                    vel = Velocity(vel.GetSpeed().first, -vel.GetSpeed().second);  // Inverser la vitesse en Y
                }
                if (newPosY >= screenHeight - pos.GetSize().second) { // Collision avec le bas
                    // Vérifier si c'est la dernière balle
                    bool isLastBall = true;
                    for (const auto& otherEntity : entities) {
                        if (otherEntity->hasComponent<Comportement>() && otherEntity->getComponent<Comportement>().GetType() == Comportement::TypesComportement::Balle && otherEntity != entity) {
                            isLastBall = false;
                            break;
                        }
                    }

                    if (!isLastBall) {
                        // Supprimer l'entité si ce n'est pas la dernière balle
                        it = entities.erase(it);
                        continue;
                    }
                    else {
                        // Réinitialiser la position et la vitesse si c'est la dernière balle
                        pos = Position2D(400, 540, pos.GetSize().first, pos.GetSize().second);
                        vitesseBalle = 0.1f;
                        float newVy = vitesseBalle * -1;
                        float newVx = vitesseBalle * (rand() % 2 == 0 ? 1 : -1);
                        vel = Velocity(newVx, newVy);
                        break;
                    }
                }

                for (auto& otherEntity : entities) {
                    if (otherEntity->hasComponent<Comportement>() &&
                        otherEntity->getComponent<Comportement>().GetType() == Comportement::TypesComportement::Brique) {

                        auto& brickPos = otherEntity->getComponent<Position2D>();
                        auto brickPosition = brickPos.GetPosition();
                        auto brickSize = brickPos.GetSize();

                        // Vérification de la collision (simple boîte englobante)
                        if (newPosX < brickPosition.first + brickSize.first &&
                            newPosX + pos.GetSize().first > brickPosition.first &&
                            newPosY < brickPosition.second + brickSize.second &&
                            newPosY + pos.GetSize().second > brickPosition.second) {

                            // Inverser la direction de la balle
                            score += 100;
                            vitesseBalle += 0.0005;
                            vel = Velocity(vitesseBalle, -vel.GetSpeed().second);

                            // Supprimer la brique
                            entities.erase(std::remove(entities.begin(), entities.end(), otherEntity), entities.end());
                            break;
                        }
                    }
                }

                for (auto& otherEntity : entities) {
                    if (otherEntity->hasComponent<Comportement>() &&
                        otherEntity->getComponent<Comportement>().GetType() == Comportement::TypesComportement::Pad) {

                        auto& paddlePos = otherEntity->getComponent<Position2D>();
                        auto paddlePosition = paddlePos.GetPosition();
                        auto paddleSize = paddlePos.GetSize();

                        // Vérification de la collision avec la raquette
                        if (newPosX < paddlePosition.first + paddleSize.first &&
                            newPosX + pos.GetSize().first > paddlePosition.first &&
                            newPosY + pos.GetSize().second >= paddlePosition.second && // vérifie que la balle arrive par le haut
                            newPosY < paddlePosition.second + paddleSize.second) {

                            // Inverser la direction verticale de la balle
                            vitesseBalle += 0.005;
                            vel = Velocity(vitesseBalle, -vitesseBalle);
                            std::srand(static_cast<unsigned int>(std::time(0)));
                            float paddlemiddle = paddlePosition.first + paddleSize.first / 2;
                            float randomValue = 0.1f + (static_cast<float>(std::rand()) / RAND_MAX) * (0.3f - 0.1f);
                            if (newPosX + pos.GetSize().first / 2 < paddlemiddle) {
                                vel = Velocity(-vitesseBalle * randomValue, -vitesseBalle);
                            }
                            else if (newPosX + pos.GetSize().first / 2 == paddlemiddle) {
                                vel = Velocity(-vitesseBalle, -vitesseBalle);
                            }
                            else {
                                vel = Velocity(vitesseBalle * randomValue, -vitesseBalle);
                            }
                        }
                    }
                }

                // Mise à jour de la position 
                pos.SetPosition(newPosX, newPosY);
            }
            ++it;
        }

        if (isLevelComplete(*this) == true ) {
            // Augmentation du niveau et de la difficulté
            niveau++;
            vitesseBalle += 0.05f;  // Augmentation de la vitesse de la balle

            // Réinitialisation des briques pour le prochain niveau
            resetBricks();
            resetBall();
        }
    }

    void resetBricks() {
        // Effacer les briques existantes
        entities.erase(std::remove_if(entities.begin(), entities.end(),[](const std::shared_ptr<Entités>& entity) {
                return entity->hasComponent<Comportement>() && entity->getComponent<Comportement>().GetType() == Comportement::TypesComportement::Brique; }), entities.end());

       
        int colonnes = 11; // Nombre de briques par ligne
        int lignes = 3+niveau;  // Nombre de lignes de briques
        float largeurBrique = 60.f;
        float hauteurBrique = 20.f;
        float espacementX = 5.f;
        float espacementY = 5.f;
        float startX = 50.f;
        float startY = 15.f;

        for (int i = 0; i < lignes; ++i) {
            for (int j = 0; j < colonnes; ++j) {
                float posX = startX + j * (largeurBrique + espacementX);
                float posY = startY + i * (hauteurBrique + espacementY);

                auto brique = std::make_shared<Entités>();
                brique->addComponent(Position2D(posX, posY, largeurBrique, hauteurBrique));
                brique->addComponent(StyleForme(StyleForme::TypeFormes::Rectangle, sf::Color::White));
                brique->addComponent(Comportement(Comportement::TypesComportement::Brique));
                addEntity(brique);
            }
        }
    }


    void resetBall() {
        // Réinitialiser la balle au centre avec une nouvelle vitesse
        for (auto& entity : entities) {
            if (entity->hasComponent<Comportement>() &&
                entity->getComponent<Comportement>().GetType() == Comportement::TypesComportement::Balle) {
                auto& pos = entity->getComponent<Position2D>();
                auto& vel = entity->getComponent<Velocity>();

                // Position au centre et vitesse avec la nouvelle difficulté
                pos.SetPosition(400.f, 300.f);
                vel = Velocity(vitesseBalle, -vitesseBalle);
            }
        }
    }

    int getScore() const {
        return score;
    }

    const std::vector<std::shared_ptr<Entités>>& getEntities() const {
        return entities;
    }
};


int main() {
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Casse-briques");

    EntityManager manager;

    // pour le score
    sf::Font font;
    if (!font.loadFromFile("ARIAL.TTF")) {
        std::cout << "Erreur de chargement de la police!" << std::endl;
        return -1;
    }

    sf::Text scoreText;
    scoreText.setFont(font);
    scoreText.setCharacterSize(24);
    scoreText.setFillColor(sf::Color::White);
    scoreText.setPosition(10, 10);

    // Création de la balle
    auto ball = std::make_shared<Entités>();
    ball->addComponent(Position2D(400.f, 300.f, 10.f, 10.f));
    ball->addComponent(Velocity(0.05f, 0.05f));
    ball->addComponent(StyleForme(StyleForme::TypeFormes::Cercle, sf::Color::White));
    ball->addComponent(Comportement(Comportement::TypesComportement::Balle));
    manager.addEntity(ball);

    // Création de la raquette
    auto paddle = std::make_shared<Entités>();
    paddle->addComponent(Position2D(400.f, 550.f, 100.f, 20.f)); // Position proche du bas
    paddle->addComponent(StyleForme(StyleForme::TypeFormes::Rectangle, sf::Color::White));
    paddle->addComponent(Comportement(Comportement::TypesComportement::Pad));
    manager.addEntity(paddle);

    // Paramètres pour la grille de briques
    int briquesParLigne = 11; // Nombre de briques par ligne
    int lignesDeBriques = 3;  // Nombre de lignes de briques
    float largeurBrique = 60.f;
    float hauteurBrique = 20.f;
    float espacementX = 5.f;
    float espacementY = 5.f;
    float debutX = 50.f;
    float debutY = 15.f;

    // Création des briques
    for (int i = 0; i < lignesDeBriques; ++i) {
        for (int j = 0; j < briquesParLigne; ++j) {
            float posX = debutX + j * (largeurBrique + espacementX);
            float posY = debutY + i * (hauteurBrique + espacementY);

            auto brique = std::make_shared<Entités>();
            brique->addComponent(Position2D(posX, posY, largeurBrique, hauteurBrique));
            brique->addComponent(StyleForme(StyleForme::TypeFormes::Rectangle, sf::Color::White));
            brique->addComponent(Comportement(Comportement::TypesComportement::Brique));
            manager.addEntity(brique);
        }
    }

    const float paddleSpeed = 0.3f; 


    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // Mise à jour des entités
        manager.updateEntities(WINDOW_WIDTH, WINDOW_HEIGHT);

        window.clear();

        // Rendu des entités
        for (const auto& entity : manager.getEntities()) {
            if (entity->hasComponent<Position2D>() && entity->hasComponent<StyleForme>()) {
                auto pos = entity->getComponent<Position2D>().GetPosition();
                auto style = entity->getComponent<StyleForme>();

                if (style.forme == StyleForme::TypeFormes::Cercle) {
                    sf::CircleShape circle(10.f);
                    circle.setPosition(pos.first, pos.second);
                    circle.setFillColor(style.color);
                    window.draw(circle);
                }
                else if (style.forme == StyleForme::TypeFormes::Rectangle) { 
                    auto size = entity->getComponent<Position2D>().GetSize(); 
                    sf::RectangleShape rectangle(sf::Vector2f(size.first, size.second)); 
                    rectangle.setPosition(pos.first, pos.second); 
                    rectangle.setFillColor(style.color); 
                    window.draw(rectangle); 
                }

            }
            if (entity->hasComponent<Comportement>() && entity->getComponent<Comportement>().GetType() == Comportement::TypesComportement::Pad) {

                auto& pos = entity->getComponent<Position2D>();

                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
                    pos = Position2D(pos.GetPosition().first - paddleSpeed, pos.GetPosition().second, pos.GetSize().first, pos.GetSize().second);
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
                    pos = Position2D(pos.GetPosition().first + paddleSpeed, pos.GetPosition().second, pos.GetSize().first, pos.GetSize().second);
                }

                // Limiter le mouvement de la raquette pour qu'elle ne sorte pas de l'écran
                if (pos.GetPosition().first < 0.f) {
                    pos = Position2D(0.f, pos.GetPosition().second, pos.GetSize().first, pos.GetSize().second);
                }
                if (pos.GetPosition().first > 800.f - pos.GetSize().first) {
                    pos = Position2D(800.f - pos.GetSize().first, pos.GetPosition().second, pos.GetSize().first, pos.GetSize().second);
                }
            }
        }
        // Affichage du score
        
        scoreText.setString("Score: " + std::to_string(manager.getScore()));
        window.draw(scoreText);
        window.display();
    }

    return 0;
}
