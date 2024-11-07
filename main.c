#include "raylib.h"
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <raymath.h>

#define WINDOWWIDTH 800
#define WINDOWHEIGHT 600
#define INVINCIBILITY_DURATION 2.0f
#define MAX_PLAYER_BULLETS 512
#define MAX_ENEMY_BULLETS 12
#define MAX_ENEMIES 500
#define ENEMY_BULLET_SPEED 500.0f
#define PLAYER_BULLET_SPEED 700.0f
#define BULLET_ARC 90.0f
#define BULLET_ARC_COUNT 15
#define SHOOT_COOLDOWN 0.15f

#define MAX_ZOMBIES 50
#define MAX_SKELETONS 50
#define MAX_ORCS 20
#define MAX_DRAGONS 2
#define GLOBAL_ENEMY_LIMIT 100

#define SKELETON_SHOOT_INTERVAL 1.0f
#define SKELETON_SHOOT_DISTANCE 300.0f
#define DRAGON_VELOCITY 250.0f

int active_zombies = 0;
int active_skeletons = 0;
int active_orcs = 0;
int active_dragons = 0;
int total_active_enemies = 0;

int PLAYER_LIFE = 3;
bool isGameOver = false;
int POWER = 0;
int SCORE = 0;

int g_bullet_wave_triggers = 0;
float g_time_since_last_wave = 0.0f;

typedef enum
{
    IDLE,
    HURT
} PlayerState;

typedef struct
{
    Vector2 position;
    Vector2 velocity;
    bool active;
} player_bullet;

typedef struct
{
    Vector2 position;
    Vector2 velocity;
    bool active;
} enemy_bullet;

typedef struct
{
    Vector2 position;
    float velocity;
    PlayerState state;
    float invincibility_timer; // Tracks the invincibility duration
    float shoot_cooldown;
} Player;

typedef struct
{
    Vector2 position;
    Vector2 velocity;
    int health;
    int size;
    bool active;
    float respawn_timer;
    Color color;
    float damage_indicator;
} Enemy;

Player player = {
    .position.x = 400,
    .position.y = 300,
    .velocity = 350,
    .state = IDLE,
    .invincibility_timer = 0,
    .shoot_cooldown = 0};

Enemy zombie = {
    .velocity = 100,
    .health = 2,
    .position.x = 0,
    .position.y = 0,
    .size = 15};

Enemy skeleton = {
    .velocity = 150,
    .health = 1,
    .position.x = 800,
    .position.y = 0,
    .size = 15};

Enemy orc = {
    .velocity = 70,
    .health = 5,
    .position.x = 0,
    .position.y = 600,
    .size = 24};

Enemy dragon = {
    .velocity = 150,
    .health = 20,
    .position.x = 800,
    .position.y = 600,
    .size = 45};

player_bullet player_bullets[MAX_PLAYER_BULLETS];
Enemy enemies[MAX_ENEMIES];
enemy_bullet skeleton_bullets[MAX_ENEMY_BULLETS];

void init_enemy(Enemy *enemy, Vector2 position, Vector2 velocity, int health, int size, Color color)
{
    enemy->position = position;
    enemy->velocity = velocity;
    enemy->health = health;
    enemy->size = size;
    enemy->color = color;                          // Assign color
    enemy->active = true;                          // Mark as active
    enemy->respawn_timer = GetRandomValue(10, 20); // Random respawn timer (5 to 20 seconds)
}

void init_enemy_bullets()
{
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++)
    {
        skeleton_bullets[i].active = false;
    }
}

void shoot_skeleton_bullet(Vector2 position, Vector2 direction)
{
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++)
    {
        if (!skeleton_bullets[i].active)
        {
            skeleton_bullets[i].position = position;
            skeleton_bullets[i].velocity = Vector2Scale(direction, ENEMY_BULLET_SPEED); // Set bullet velocity
            skeleton_bullets[i].active = true;
            break;
        }
    }
}

void update_skeleton_bullets(float delta_time)
{
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++)
    {
        if (skeleton_bullets[i].active)
        {
            skeleton_bullets[i].position.x += skeleton_bullets[i].velocity.x * delta_time;
            skeleton_bullets[i].position.y += skeleton_bullets[i].velocity.y * delta_time;

            // Deactivate the bullet if it goes off screen
            if (skeleton_bullets[i].position.x < 0 || skeleton_bullets[i].position.x > WINDOWWIDTH ||
                skeleton_bullets[i].position.y < 0 || skeleton_bullets[i].position.y > WINDOWHEIGHT)
            {
                skeleton_bullets[i].active = false;
            }
        }
    }
}

void draw_skeleton_bullets()
{
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++)
    {
        if (skeleton_bullets[i].active)
        {
            DrawCircleV(skeleton_bullets[i].position, 5, RED); // Draw bullets as red circles
        }
    }
}

void spawn_enemy(int enemy_index)
{
    if (total_active_enemies >= GLOBAL_ENEMY_LIMIT)
    {
        return; // Do not spawn if we've reached the global limit
    }

    // Define the spawn positions for corners
    Vector2 spawn_positions[4] = {
        {0, 0},                     // Top-left
        {WINDOWWIDTH, 0},           // Top-right
        {0, WINDOWHEIGHT},          // Bottom-left
        {WINDOWWIDTH, WINDOWHEIGHT} // Bottom-right
    };

    // Randomly choose a corner to spawn the enemy
    int corner_index = GetRandomValue(0, 3);
    Vector2 spawn_position = spawn_positions[corner_index];

    // Set different types of enemies with varying sizes and colors
    switch (GetRandomValue(0, 3))
    {
    case 0:                               // Zombie
        if (active_zombies < MAX_ZOMBIES) // Check the zombie limit
        {
            Vector2 zombie_velocity = {100.0f, 100.0f}; // Example: moving right and down
            init_enemy(&enemies[enemy_index], spawn_position, zombie_velocity, 2, 20, DARKGREEN);
            active_zombies++;
            total_active_enemies++;
        }
        break;
    case 1:                                   // Skeleton
        if (active_skeletons < MAX_SKELETONS) // Check the skeleton limit
        {
            Vector2 skeleton_velocity = {150.0f, 150.0f}; // Example: moving right and up
            init_enemy(&enemies[enemy_index], spawn_position, skeleton_velocity, 1, 15, LIGHTGRAY);
            active_skeletons++;
            total_active_enemies++;
        }
        break;
    case 2:                         // Orc
        if (active_orcs < MAX_ORCS) // Check the orc limit
        {
            Vector2 orc_velocity = {80.0f, 80.0f}; // Example: moving right
            init_enemy(&enemies[enemy_index], spawn_position, orc_velocity, 5, 30, DARKGRAY);
            active_orcs++;
            total_active_enemies++;
        }
        break;
    case 3:                               // Dragon
        if (active_dragons < MAX_DRAGONS) // Check the dragon limit
        {
            Vector2 dragon_velocity = {70.0f, 70.0f}; // Example: moving diagonally
            init_enemy(&enemies[enemy_index], spawn_position, dragon_velocity, 20, 45, RED);
            active_dragons++;
            total_active_enemies++;
        }
        break;
    }
}

void update_enemies(float delta_time)
{
    static float skeleton_shoot_timer = 0.0f; // Timer for skeleton shooting

    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        if (enemies[i].active)
        {
            if (enemies[i].damage_indicator > 0)
            {
                enemies[i].damage_indicator -= delta_time;
                if (enemies[i].damage_indicator < 0)
                {
                    enemies[i].damage_indicator = 0;
                }
            }

            if (enemies[i].health > 0) // Only move if the enemy is alive
            {
                // Skeleton behavior
                if (enemies[i].size == 15) // Assuming skeleton size is 15
                {
                    Vector2 direction = Vector2Subtract(player.position, enemies[i].position);
                    float distance = Vector2Length(direction);

                    // Maintain a fixed distance from the player
                    if (distance > SKELETON_SHOOT_DISTANCE)
                    {
                        direction = Vector2Normalize(direction);
                        enemies[i].position.x += direction.x * enemies[i].velocity.x * delta_time;
                        enemies[i].position.y += direction.y * enemies[i].velocity.y * delta_time;
                    }
                    else
                    {
                        // Shoot bullets at the player if within the specified range
                        skeleton_shoot_timer -= delta_time;
                        if (skeleton_shoot_timer <= 0.0f)
                        {
                            shoot_skeleton_bullet(enemies[i].position, Vector2Normalize(direction)); // Shoot towards player
                            skeleton_shoot_timer = SKELETON_SHOOT_INTERVAL;                          // Reset the timer
                        }
                    }
                }

                // Dragon behavior
                else if (enemies[i].size == 45)
                {
                    if (enemies[i].velocity.x == 0 && enemies[i].velocity.y == 0)
                    {
                        enemies[i].velocity.x = DRAGON_VELOCITY; // Initial X velocity
                        enemies[i].velocity.y = DRAGON_VELOCITY; // Initial Y velocity
                    }

                    // Move the dragon according to its velocity
                    enemies[i].position.x += enemies[i].velocity.x * delta_time;
                    enemies[i].position.y += enemies[i].velocity.y * delta_time;

                    // Reverse velocity when dragon hits the screen bounds (bounce effect)
                    if (enemies[i].position.x <= 0 || enemies[i].position.x >= WINDOWWIDTH)
                    {
                        enemies[i].velocity.x *= -1; // Reverse X direction
                    }
                    if (enemies[i].position.y <= 0 || enemies[i].position.y >= WINDOWHEIGHT)
                    {
                        enemies[i].velocity.y *= -1; // Reverse Y direction
                    }
                }

                // Default movement for other enemies
                else
                {
                    Vector2 direction = {player.position.x - enemies[i].position.x, player.position.y - enemies[i].position.y};
                    float length = sqrtf(direction.x * direction.x + direction.y * direction.y);
                    if (length != 0)
                    {
                        // Normalize direction
                        direction.x /= length;
                        direction.y /= length;
                    }

                    enemies[i].position.x += direction.x * enemies[i].velocity.x * delta_time;
                    enemies[i].position.y += direction.y * enemies[i].velocity.y * delta_time;
                }
            }
            else // If health is 0 or less, deactivate the enemy
            {
                enemies[i].active = false;
                total_active_enemies--;
            }
        }
    }
}

void draw_enemies()
{
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        if (enemies[i].active)
        {
            Color enemyColor = enemies[i].color;
            if (enemies[i].damage_indicator > 0)
            {
                enemyColor = RED;
            }

            DrawCircleV(enemies[i].position, enemies[i].size, enemyColor);
        }
    }
}

void init_bullets()
{
    for (int i = 0; i < MAX_PLAYER_BULLETS; i++)
    {
        player_bullets[i].active = false;
    }
}

// Shoot a bullet in the direction of the mouse cursor
void shoot_bullet(Vector2 position, float angle)
{
    float offsetDistance = 20.0f; // Distance in front of the player to spawn the bullet

    // Calculate the offset position in front of the player
    Vector2 bullet_position = {
        position.x + cosf(angle) * offsetDistance,
        position.y + sinf(angle) * offsetDistance};

    for (int i = 0; i < MAX_PLAYER_BULLETS; i++)
    {
        if (!player_bullets[i].active)
        {
            player_bullets[i].position = bullet_position;
            player_bullets[i].velocity.x = cosf(angle) * PLAYER_BULLET_SPEED;
            player_bullets[i].velocity.y = sinf(angle) * PLAYER_BULLET_SPEED;
            player_bullets[i].active = true;
            break;
        }
    }
}

float calculate_angle_to_mouse(Vector2 player_position)
{
    Vector2 mouse_position = GetMousePosition();
    float dx = mouse_position.x - player_position.x;
    float dy = mouse_position.y - player_position.y;
    return atan2f(dy, dx); // Get angle in radians between player and mouse
}

void bullet_wave(Vector2 player_position, float player_angle)
{

    float arc_radians = BULLET_ARC * DEG2RAD;

    float angle_step = arc_radians / (BULLET_ARC_COUNT - 1);

    float start_angle = player_angle - (arc_radians / 2);

    for (int i = 0; i < BULLET_ARC_COUNT; i++)
    {
        for (int j = 0; j < MAX_PLAYER_BULLETS; j++)
        {
            if (!player_bullets[j].active)
            {
                player_bullets[j].active = true;

                player_bullets[j].position = player_position;

                float bullet_angle = start_angle + (i * angle_step);

                player_bullets[j].velocity.x = PLAYER_BULLET_SPEED * cos(bullet_angle);
                player_bullets[j].velocity.y = PLAYER_BULLET_SPEED * sin(bullet_angle);

                break;
            }
        }
    }
}

void update_bullets(float delta_time)
{
    for (int i = 0; i < MAX_PLAYER_BULLETS; i++)
    {
        if (player_bullets[i].active)
        {
            // Move the bullet
            player_bullets[i].position.x += player_bullets[i].velocity.x * delta_time;
            player_bullets[i].position.y += player_bullets[i].velocity.y * delta_time;

            // Check if the bullet is off the screen
            if (player_bullets[i].position.x < 0 || player_bullets[i].position.x > WINDOWWIDTH ||
                player_bullets[i].position.y < 0 || player_bullets[i].position.y > WINDOWHEIGHT)
            {
                player_bullets[i].active = false; // Deactivate bullet if off-screen
            }
        }
    }
}

void draw_bullets()
{
    for (int i = 0; i < MAX_PLAYER_BULLETS; i++)
    {
        if (player_bullets[i].active)
        {
            DrawCircleV(player_bullets[i].position, 5, YELLOW); // Draws bullets as small red circles
        }
    }
}

void update_player(float delta_time)
{
    // Update invincibility timer
    if (player.state == HURT)
    {
        player.invincibility_timer -= delta_time;
        if (player.invincibility_timer <= 0)
        {
            player.state = IDLE;
            player.invincibility_timer = 0;
        }
    }

    if (player.shoot_cooldown > 0)
    {
        player.shoot_cooldown -= delta_time;

        if (player.shoot_cooldown < 0)
        {
            player.shoot_cooldown = 0;
        }
    }

    Vector2 movement = {0.0f, 0.0f};

    // Input
    if (IsKeyDown(KEY_D) && player.position.x < GetScreenWidth() - 15)
    {
        movement.x += 1.0f;
    }

    if (IsKeyDown(KEY_A) && player.position.x > 15)
    {
        movement.x -= 1.0f;
    }

    if (IsKeyDown(KEY_W) && player.position.y > 15)
    {
        movement.y -= 1.0f;
    }

    if (IsKeyDown(KEY_S) && player.position.y < GetScreenHeight() - 15)
    {
        movement.y += 1.0f;
    }

    // Normalize the movement vector if moving diagonally
    if (movement.x != 0.0f && movement.y != 0.0f)
    {
        movement.x *= 0.7071f;
        movement.y *= 0.7071f;
    }

    // Apply movement
    player.position.x += movement.x * player.velocity * delta_time;
    player.position.y += movement.y * player.velocity * delta_time;
}

void check_player_collision()
{
    // Check for enemy bullets hitting the player
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++)
    {
        if (skeleton_bullets[i].active)
        {
            if (CheckCollisionCircles(player.position, 20, skeleton_bullets[i].position, 5))
            {
                if (player.state == IDLE)
                {
                    player.state = HURT;
                    player.invincibility_timer = INVINCIBILITY_DURATION;
                    skeleton_bullets[i].active = false;
                    PLAYER_LIFE--;

                    if (PLAYER_LIFE <= 0)
                    {
                        isGameOver = true;
                    }
                }
            }
        }
    }

    // Check for player bullets hitting enemies
    for (int i = 0; i < MAX_PLAYER_BULLETS; i++)
    {
        if (player_bullets[i].active)
        {
            for (int j = 0; j < MAX_ENEMIES; j++)
            {
                if (enemies[j].active)
                {
                    if (CheckCollisionCircles(player_bullets[i].position, 5, enemies[j].position, enemies[j].size))
                    {
                        enemies[j].health--;
                        enemies[j].damage_indicator = 0.1f;
                        player_bullets[i].active = false;

                        if (enemies[j].health <= 0)
                        {
                            enemies[j].active = false;
                            total_active_enemies--;
                            SCORE += 10;
                            POWER += 5;
                        }
                    }
                }
            }
        }
    }

    // Check collision between player and enemy body
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        if (enemies[i].active)
        {
            if (CheckCollisionCircles(player.position, 20, enemies[i].position, enemies[i].size))
            {
                if (player.state == IDLE)
                {
                    player.state = HURT;
                    player.invincibility_timer = INVINCIBILITY_DURATION;
                    PLAYER_LIFE--;

                    if (PLAYER_LIFE <= 0)
                    {
                        isGameOver = true;
                    }
                }
            }
        }
    }
}

void draw_triangle(Vector2 position, float size, float rotation)
{
    // Move the base of the triangle a bit further from the player's center
    float offsetDistance = 30.0f; // Adjust distance from the player

    // Offset position for the base of the triangle
    Vector2 basePosition = {
        position.x + cosf(rotation) * offsetDistance,
        position.y + sinf(rotation) * offsetDistance};

    // Define triangle vertices relative to the base position
    Vector2 tip = {
        basePosition.x + cosf(rotation) * size,
        basePosition.y + sinf(rotation) * size};
    Vector2 left = {
        basePosition.x + cosf(rotation + 120 * DEG2RAD) * (size / 2),
        basePosition.y + sinf(rotation + 120 * DEG2RAD) * (size / 2)};
    Vector2 right = {
        basePosition.x + cosf(rotation - 120 * DEG2RAD) * (size / 2),
        basePosition.y + sinf(rotation - 120 * DEG2RAD) * (size / 2)};

    DrawLineV(position, basePosition, YELLOW);
    DrawLineV(basePosition, tip, YELLOW);
    DrawLineV(basePosition, left, YELLOW);
    DrawLineV(basePosition, right, YELLOW);

    // Draw the triangle
    DrawTriangle(tip, left, right, YELLOW);
}

void draw_gun(Vector2 position, float size, float rotation)
{
    // Move the base of the triangle a bit further from the player's center
    float offsetDistance = 30.0f; // Adjust distance from the player

    // Offset position for the base of the triangle
    Vector2 basePosition = {
        position.x + cosf(rotation) * offsetDistance,
        position.y + sinf(rotation) * offsetDistance};

    // Define triangle vertices relative to the base position
    Vector2 tip = {
        basePosition.x + cosf(rotation) * size,
        basePosition.y + sinf(rotation) * size};
    Vector2 left = {
        basePosition.x + cosf(rotation + 120 * DEG2RAD) * (size / 2),
        basePosition.y + sinf(rotation + 120 * DEG2RAD) * (size / 2)};
    Vector2 right = {
        basePosition.x + cosf(rotation - 120 * DEG2RAD) * (size / 2),
        basePosition.y + sinf(rotation - 120 * DEG2RAD) * (size / 2)};

    DrawLineV(position, basePosition, YELLOW);
    DrawLineV(basePosition, tip, YELLOW);
    DrawLineV(basePosition, left, YELLOW);
    DrawLineV(basePosition, right, YELLOW);

    // Draw the gun
    DrawTriangle(tip, left, right, YELLOW);
}

void draw_player()
{
    Color player_color = WHITE;
    if (player.state == HURT && ((int)(player.invincibility_timer * 10) % 2 == 0)) // Flashing effect
    {
        player_color = DARKGRAY; // Flash color when hurt
    }
    DrawCircleV(player.position, 20, player_color); // Draw player as a circle
}

int main()
{
    InitWindow(800, 600, "Hordeshooter");

    srand(time(NULL));
    SetTargetFPS(60);

    init_bullets();
    init_enemy_bullets(); // Initialize skeleton bullets

    // Spawn initial enemies
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        enemies[i].active = false;
    }

    while (!WindowShouldClose())
    {
        float delta_time = GetFrameTime();
        BeginDrawing();
        ClearBackground(BLACK);
        // Draw score and lives
        DrawText(TextFormat("Lives: %d", PLAYER_LIFE), 10, 10, 20, WHITE);
        DrawText(TextFormat("Score: %d", SCORE), 10, 40, 20, WHITE);
        DrawText(TextFormat("Power: %d", POWER), 320, 540, 40, WHITE);

        if (!isGameOver)
        {
            update_player(delta_time);
            if (g_bullet_wave_triggers > 0)
            {
                if (g_time_since_last_wave > 10.0f)
                {
                    bullet_wave(player.position, calculate_angle_to_mouse(player.position));
                    g_bullet_wave_triggers--;
                }
            }
            else
            {
                g_time_since_last_wave += GetFrameTime();
            }
            update_enemies(delta_time);          // Update enemies
            update_bullets(delta_time);          // Update bullets
            update_skeleton_bullets(delta_time); // Update skeleton bullets

            // Get the mouse position
            Vector2 mouse_position = GetMousePosition();

            // Calculate the angle between the player and the mouse cursor
            float angle = atan2f(mouse_position.y - player.position.y, mouse_position.x - player.position.x);

            draw_gun(player.position, 30.0f, angle);
            draw_bullets();
            draw_enemies();
            draw_skeleton_bullets();
            draw_player();
            check_player_collision();

            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && player.shoot_cooldown <= 0)
            {
                shoot_bullet(player.position, angle);
                player.shoot_cooldown = SHOOT_COOLDOWN; // Reset cooldown after shooting
            }

            if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && POWER >= 100)
            {
                g_bullet_wave_triggers = 3;
                POWER = 0;
            }
        }
        else
        {
            if (IsKeyPressed(KEY_R))
            {
                isGameOver = false;
                PLAYER_LIFE = 3; // Reset player lives
                SCORE = 0;       // Reset score
                active_zombies = 0;
                active_skeletons = 0;
                active_orcs = 0;
                active_dragons = 0;       // Reset enemy counts
                total_active_enemies = 0; // Reset total enemy count
                for (int i = 0; i < MAX_ENEMIES; i++)
                {
                    enemies[i].active = false; // Deactivate all enemies
                }
            }
            DrawText("GAME OVER", WINDOWWIDTH / 2 - 120, WINDOWHEIGHT / 2 - 30, 40, RED);
            DrawText(TextFormat("Your score was: %d", SCORE), WINDOWWIDTH / 2 - 95, WINDOWHEIGHT / 2 + 10, 20, DARKGRAY);
            DrawText("Press R to Restart", WINDOWWIDTH / 2 - 100, WINDOWHEIGHT / 2 + 30, 20, DARKGRAY);
        }

        if (GetRandomValue(0, 60) < 2) // Adjust this for enemy spawn frequency
        {
            int enemy_index = GetRandomValue(0, MAX_ENEMIES - 1);
            spawn_enemy(enemy_index);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
