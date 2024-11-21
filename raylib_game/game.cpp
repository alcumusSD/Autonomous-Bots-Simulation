#include "raylib.h"
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <string>

using namespace std;

struct Ship {
    Vector2 position;
    float speed;
    float rotation;
    Texture2D texture;
    float width;
    float height;
    Vector2 velocity;
    float maxForce;
    float detectionRadius;
    float wanderAngle;
    bool seeking;
    int points;
    float sizeMultiplier;
    Color color;

    Ship(Vector2 startPos, float startSpeed, Texture2D tex, float texWidth, float texHeight, Color shipColor)
        : position(startPos), speed(startSpeed), rotation(0.0f), texture(tex), width(texWidth), height(texHeight),
        velocity({ 0, 0 }), maxForce(0.1f), detectionRadius(150.0f), wanderAngle(0.0f), seeking(false), points(0),
        sizeMultiplier(1.0f), color(shipColor) {}

    void applyForce(Vector2 force) {
        velocity.x += force.x;
        velocity.y += force.y;

        float velocityMag = sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
        if (velocityMag > speed) {
            velocity.x = (velocity.x / velocityMag) * speed;
            velocity.y = (velocity.y / velocityMag) * speed;
        }
    }

    void arrive(Vector2 target) {
        Vector2 desired = { target.x - position.x, target.y - position.y };
        float distance = sqrt(pow(desired.x, 2) + pow(desired.y, 2));

        if (distance > 0) {
            desired.x /= distance;
            desired.y /= distance;

            if (distance < 100) {
                speed = distance / 100 * 5;
            }
            else {
                speed = 5.0f;
            }
        }
        else {
            desired = { 0, 0 };
        }

        Vector2 desiredVelocity = { desired.x * speed, desired.y * speed };
        Vector2 steer = { desiredVelocity.x - velocity.x, desiredVelocity.y - velocity.y };
        float steerMag = sqrt(steer.x * steer.x + steer.y * steer.y);

        if (steerMag > maxForce) {
            float angle = atan2(steer.y, steer.x);
            steer.x = cos(angle) * maxForce;
            steer.y = sin(angle) * maxForce;
        }

        applyForce(steer);

        position.x += velocity.x;
        position.y += velocity.y;

        rotation = atan2f(velocity.y, velocity.x) * RAD2DEG;
    }

    void wander() {
        wanderAngle += ((rand() % 200) - 100) * 0.01f;

        Vector2 wanderForce = {
            cos(wanderAngle) * 2.0f,
            sin(wanderAngle) * 2.0f
        };

        applyForce(wanderForce);
    }

    void draw() const {
        Vector2 origin = { (width * sizeMultiplier) / 2, (height * sizeMultiplier) / 2 };
        DrawTexturePro(texture,
            { 0, 0, width, height },
            { position.x, position.y, width * sizeMultiplier, height * sizeMultiplier },
            origin,
            rotation,
            color);
    }


    bool isFoodInRange(Vector2 foodPosition) const {
        float distance = sqrt(pow(position.x - foodPosition.x, 2) + pow(position.y - foodPosition.y, 2));
        return distance <= detectionRadius;
    }

    void increaseSize(float amount) {
        sizeMultiplier += amount;
    }

    void increaseDetectionRadius(float amount) {
        detectionRadius += amount;
    }
    void mutate(Ship& otherShip) {
        points += otherShip.points;
        sizeMultiplier += otherShip.sizeMultiplier * 0.5f;

        velocity.x = (velocity.x + otherShip.velocity.x) / 2;
        velocity.y = (velocity.y + otherShip.velocity.y) / 2;

        color.r = (color.r + otherShip.color.r) / 2;
        color.g = (color.g + otherShip.color.g) / 2;
        color.b = (color.b + otherShip.color.b) / 2;

        increaseDetectionRadius(20.0f);

        otherShip.position = { -9999, -9999 };
    }
};


void checkForMutation(vector<Ship>& ships) {
    for (size_t i = 0; i < ships.size(); i++) {
        for (size_t j = i + 1; j < ships.size(); j++) {
            float distance = sqrt(pow(ships[i].position.x - ships[j].position.x, 2) +
                pow(ships[i].position.y - ships[j].position.y, 2));
            if (distance < 20.0f) {
                ships[i].mutate(ships[j]);
                ships.erase(ships.begin() + j);
                break;
            }
        }
    }
}


struct Food {
    Vector2 position;
    int points;
    Color color;
    float radius;
    bool active;

    Food(Vector2 pos, int value, Color c, float r) : position(pos), points(value), color(c), radius(r), active(true) {}
};

vector<Food> generateRandomFood(int screenWidth, int screenHeight) {
    int foodCount = rand() % 100 + 50;

    vector<Food> foods;
    for (int i = 0; i < foodCount; i++) {
        Vector2 pos = {
            static_cast<float>(rand() % screenWidth),
            static_cast<float>(rand() % screenHeight)
        };

        int points = rand() % 100 + 1;

        Color color;
        color = DARKGREEN;

        float radius = 15.0;

        foods.push_back(Food(pos, points, color, radius));
    }
    return foods;
}

void resetFoodPos(vector<Food>& foods, int screenWidth, int screenHeight) {
    for (auto& food : foods) {
        food.active = true;
        food.position = {
            static_cast<float>(rand() % screenWidth),
            static_cast<float>(rand() % screenHeight)
        };
    }
}

Food* findHighestPointFoodInRange(Ship& ship, vector<Food>& foods) {
    Food* highestPointFood = nullptr;
    for (auto& food : foods) {
        if (food.active && ship.isFoodInRange(food.position)) {
            if (highestPointFood == nullptr || food.points > highestPointFood->points) {
                highestPointFood = &food;
            }
        }
    }
    return highestPointFood;
}

void eliminateWeakestShip(vector<Ship>& ships) {
    if (ships.size() > 1) {
        auto weakestShip = std::min_element(ships.begin(), ships.end(), [](const Ship& a, const Ship& b) {
            return a.points < b.points;
            });

        ships.erase(weakestShip);
    }
}

void updateShip(Ship& ship, vector<Food>& foods) {
    Food* targetFood = findHighestPointFoodInRange(ship, foods);

    if (targetFood) {
        ship.arrive(targetFood->position);
        float distanceToFood = sqrt(pow(ship.position.x - targetFood->position.x, 2) +
            pow(ship.position.y - targetFood->position.y, 2));
        if (distanceToFood < targetFood->radius) {
            targetFood->active = false;
            ship.points += targetFood->points;
            ship.increaseSize(0.02f); // increase Size visual cue
        }
    }
    else {
        ship.wander();
    }

    ship.draw();
}

int main() {
    srand(static_cast<unsigned int>(time(0)));
    int screenWidth = 800;
    int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "Food Targeting Game");

    Texture2D shipTexture = LoadTexture("resources/ship.png");

    float shipWidth = (float)shipTexture.width;
    float shipHeight = (float)shipTexture.height;

    vector<Color> shipColors = { RED, BLUE };

    vector<Ship> ships;
    for (int i = 0; i < 5; i++) {
        Vector2 startPosition = { screenWidth / 2 + 200 * (i - 2), screenHeight / 2 };
        ships.emplace_back(startPosition, 5.0f, shipTexture, shipWidth, shipHeight, shipColors[i % shipColors.size()]);
    }

    vector<Food> foods = generateRandomFood(screenWidth, screenHeight);
    int round = 1;

    float resetTime = 20.0f;
    float elapsedTime = 0.0f;

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        elapsedTime += GetFrameTime();

        BeginDrawing();
        ClearBackground(WHITE);
        checkForMutation(ships);


        for (auto& ship : ships) {
            updateShip(ship, foods);
        }

        for (const auto& food : foods) {
            if (food.active) {
                DrawCircleV(food.position, food.radius, food.color);
            }
        }

        int yOffset = 10;
        for (int i = 0; i < ships.size(); i++) {
            string pointsText = "Ship " + to_string(i + 1) + " Points: " + to_string(ships[i].points); //text viusal cue
            DrawText(pointsText.c_str(), 10, yOffset, 20, ships[i].color);
            yOffset += 30;
        }

        string shipsRemainingText = "Ships Remaining: " + to_string(ships.size()); //text visual cue
        DrawText(shipsRemainingText.c_str(), 10, screenHeight - 30, 20, BLACK);

        if (elapsedTime >= resetTime) {
            resetFoodPos(foods, screenWidth, screenHeight);
            eliminateWeakestShip(ships);
            elapsedTime = 0.0f;
        }

        EndDrawing();
    }

    UnloadTexture(shipTexture);
    CloseWindow();

    return 0;
}
