#include<easyx.h>
#include<vector>
#include<string>
#include<time.h>
#include <fstream>

using namespace std;

#define BULLET_NUM 2000 //���������ӵ���
#define ENEMY_NUM 10 //��������л���
#define ENEMY_SMALL	  0 //С�͵л�
//���͵л�
#define ENEMY_MEDIUM 1 
//���͵л�
#define ENEMY_BIG    2
#define AIRDROP_NUM 4 //��Ͷ����
#define AIRDROP_BLACKHOLE 0  // �ڶ��ӵ���Ͷ
#define AIRDROP_SHIELD    1  // ���ܿ�Ͷ
#define AIRDROP_HEALTH    2  // ��Ѫ��Ͷ (����)
bool is_blackhole_bullet = false; // �Ƿ�ʹ�úڶ��ӵ�
DWORD blackhole_bullet_end_time = 0; // �ڶ��ӵ�����ʱ��
bool is_invincible = false; // �������Ƿ����޵�״̬
DWORD invincible_end_time = 0; // �������޵н���ʱ��
bool has_shield = false; // �Ƿ�ӵ�л���
bool is_boss_mode = false; // BOSSսģʽ��־
bool boss_defeated = false; // BOSS�Ƿ񱻻���
DWORD boss_defeat_time = 0; // BOSS�����ܵ�ʱ��
int boss_explosion_frame = 0; // BOSS��ը����֡������
struct Boss {
    int x;
    int y;
    int width;
    int height;
    int hp;
    int max_hp;
    bool alive;
    int move_direction; // 1: ����, -1: ����
} boss1;

struct Airdrop {
    int x;
    int y;
    int width;
    int height;
    bool alive;
    int type; // 0: �ڶ��ӵ�
};
bool restart=false;

enum GameState {
    START_SCREEN,    // ��ʼ����
    INFINITE_MODE,   // �޾�ģʽ
    LEVEL_MODE,      // ����ģʽ(δ����)
    BOSS_MODE,       // BOSSģʽ(δ����)
    GAME_OVER        // ��Ϸ����
};

GameState current_state = START_SCREEN; // ��ǰ��Ϸ״̬����ʼΪ��ʼ����
bool show_unavailable_msg = false;      // �Ƿ���ʾ"δ����"��ʾ
DWORD msg_show_time = 0;                // ��ʾ��Ϣ��ʾʱ��

enum window
{
	width = 480,
	height = 600,
};
int score = 0; // ����
int high_score = 0; // ��ʷ��߷���
int bullet_level = 1; // �����������ȼ�����ʼΪ1
const string HIGH_SCORE_FILE = "./high_score.dat"; // ��߷ֱ����ļ�·��
static int player_fall_cnt = 0; //����������������
static vector<int> enemy_fall_cnt(ENEMY_NUM); //�л�������������
//�л���������
struct plane
{
	int x;
	int y;
	int width;
	int height;
	bool alive;
	bool fall;
	int hp;
	int type;
	float angle; // �������ӵ����нǶȣ����ȣ�
}player;

vector<IMAGE> image_player(5);//�����ṹ�����飨����+4֡��ը��
vector<IMAGE> image_airdrop(AIRDROP_NUM);//��Ͷ
vector<plane> enemy(ENEMY_NUM);//�л��ṹ������
vector<IMAGE> image_enemy(5);//�л���Ƭ��С��+����+���ͣ�
// �л�������ͼƬ��С�ͣ�
vector<IMAGE> image_enemy_fall_1(4);
// �л�������ͼƬ�����ͣ�
vector<IMAGE> image_enemy_fall_2(4);
// �л�������ͼƬ�����ͣ�
vector<IMAGE> image_enemy_fall_3(6);
vector<IMAGE> image_boss1(4); // BOSSͼƬ
vector<plane>bullet(BULLET_NUM);//�ӵ��ṹ������
IMAGE image_bullet;// �ӵ�ͼƬ
IMAGE image_blackhole_bullet;// �ڶ��ӵ�ͼƬ
vector<IMAGE>image_boss2(5);//boss2ͼƬ
vector<Airdrop> airdrops(AIRDROP_NUM); // ��Ͷ�ṹ������
IMAGE image_background;//����ͼƬ

// ������ťͼƬ
IMAGE image_restart;
void drawAlpha(IMAGE* picture, int picture_x, int picture_y);
bool timer_arrive(int id, int ms);

void game_init()
{
	//��ʼ����ҷɻ�
	player.hp=3;
	player.width = 80;
	player.height = 80;
	player.x = width / 2 - player.width / 2;
	player.y = height - player.height;
	player.alive = true;
	player.fall = false;
	score = 0; // ��������
	// ���õ����ȼ�
	bullet_level = 1; // ��Ӵ��������õ����ȼ�
	// ��ʼ��BOSS
	boss1.width = 150;
	boss1.height = 300;
	boss1.x = width / 2 - boss1.width / 2;
	boss1.y = 50;
	boss1.hp = 100;
	boss1.max_hp = 100;
	boss1.alive = false;
	boss1.move_direction = 1; // ��ʼ�����ƶ�
	
	//��ʼ���л�
	int i = 0;
	for (i = 0; i < ENEMY_NUM; i++)
	{
		enemy[i].alive = false;
	}
	//��ʼ���ӵ�
	for (int i = 0; i < BULLET_NUM; i++)
	{
		bullet[i].alive = false;
		bullet[i].width = 5;
		bullet[i].height = 15;
	}
	
	// ��ʼ����Ͷ
	for (int i = 0; i < AIRDROP_NUM; i++) {
		airdrops[i].alive = false;
		airdrops[i].type = 0; // Ĭ�Ϻڶ��ӵ�����
	}

	initgraph(width, height); //��ʼ������
	
	// ��ȡ�������߷�
	ifstream infile(HIGH_SCORE_FILE, ios::binary);
	if (infile.is_open())
	{
		infile.read(reinterpret_cast<char*>(&high_score), sizeof(high_score));
		infile.close();
	}
	else
	{
		high_score = 0; // �ļ�������ʱ��ʼ��Ϊ0
	}
}

//�����ӵ�����
void creat_bullet()
{
    // �ڶ��ӵ��߼� - �����������ܵ����ȼ�Ӱ��
    if (is_blackhole_bullet) {
        for (int i = 0; i < BULLET_NUM; i++) {
            if (!bullet[i].alive) {
                // ���úڶ��ӵ�����
                bullet[i].x = player.x + player.width / 2;
                bullet[i].y = player.y;
                bullet[i].alive = true;
                bullet[i].type = 1; // �ڶ��ӵ�����
                
                // �ڶ��ӵ�����Ч���������Ļ���ел�
                for (int j = 0; j < ENEMY_NUM; j++) {
                    if (enemy[j].alive) {
                        enemy[j].alive = false;
                        enemy[j].fall = true;
                        
                        // ���ݵл��������ӷ���
                        if (enemy[j].type == ENEMY_SMALL)
                            score += 10;  // �ڶ��ӵ���ɱ��������Ϊ��ͨ�ӵ���10��
                        else if (enemy[j].type == ENEMY_MEDIUM)
                            score += 20;
                        else if (enemy[j].type == ENEMY_BIG)
                            score += 30;
                    }
                }
                break;  // �ڶ��ӵ�ÿ��ֻ����1�ţ�����ȼ���������
            }
        }
    }
    // ��ͨ�ӵ��߼� - �ܵ����ȼ�Ӱ��
    else {
        // �����ӵ����������ʼλ��
        const int bullet_spacing = 25; // �ӵ����
        int start_x = player.x + (player.width - (bullet_level - 1) * bullet_spacing) / 2;
        
        // ����������4��ʱ��ʹ�����ηֲ�
        if (bullet_level > 4) {
            const float angle_range = 60.0f * 3.141592f / 180.0f; // �����νǶȣ����ȣ�
            const float start_angle = -angle_range / 2 + 1.5708f; // ��ʼ�Ƕȣ���ƫ��
            
            for (int b = 0; b < bullet_level; b++) {
                for (int i = 0; i < BULLET_NUM; i++) {
                    if (!bullet[i].alive) {
                        float angle_rad = start_angle + (angle_range * b / (bullet_level - 1));
                        bullet[i].x = player.x + player.width / 2 + cos(angle_rad) * 30;
                        bullet[i].y = player.y-sin(angle_rad) * 30;
                        bullet[i].alive = true;
                        bullet[i].type = 0; // ��ͨ�ӵ�����
                        bullet[i].angle = angle_rad; // �洢�Ƕ������ƶ�
                        break;
                    }
                }
            }
        } else {
            // 4�������µ������ֺ���ֲ�
            for (int b = 0; b < bullet_level; b++) {
                for (int i = 0; i < BULLET_NUM; i++) {
                    if (!bullet[i].alive) {
                        bullet[i].x = start_x + b * bullet_spacing;
                        bullet[i].y = player.y;
                        bullet[i].alive = true;
                        bullet[i].type = 0; // ��ͨ�ӵ�����
                        break;
                    }
                }
            }
        }
    }
}

// ���ƿ�ʼ����
void draw_start_screen() {
    // ���Ʊ���ͼƬ
    putimage(0, 0, &image_background);
    
    // ����������ʽ����ɫ
    settextstyle(40, 0, L"����");
    settextcolor(YELLOW);
    
    // ���Ʊ���"�ɻ���ս"Logo
    RECT logoRect = {0, height/4, width, height/4 + 60};
    drawtext(L"�ɻ���ս", &logoRect, DT_CENTER | DT_VCENTER);
    
    // ���Ʋ˵�ѡ��
    settextstyle(25, 0, L"����");
    settextcolor(WHITE);
    
    RECT option1Rect = {0, height/2 + 50, width, height/2 + 80};
    drawtext(L"��1�����޾�ģʽ", &option1Rect, DT_CENTER | DT_VCENTER);
    
    RECT option2Rect = {0, height/2 + 100, width, height/2 + 130};
    drawtext(L"��2���봳��ģʽ (δ����)", &option2Rect, DT_CENTER | DT_VCENTER);
    
    RECT option3Rect = {0, height/2 + 150, width, height/2 + 180};
    drawtext(L"��3����BOSSģʽ (δ����)", &option3Rect, DT_CENTER | DT_VCENTER);
    // ��ӿ��Ƽ�˵��
    settextcolor(LIGHTBLUE);  // ʹ��ǳ��ɫͻ����ʾ����˵��
    RECT controlRect1 = {0, height - 100, width, height - 70};
    drawtext(L"�ƶ�: WASD", &controlRect1, DT_CENTER | DT_VCENTER);
    
    RECT controlRect2 = {0, height - 60, width, height - 30};
    drawtext(L"����: J", &controlRect2, DT_CENTER | DT_VCENTER);
    // �����Ҫ��ʾ"��δ����"��ʾ
    if (show_unavailable_msg) {
        settextcolor(RED);
        RECT tipRect = {0, height/2 + 220, width, height/2 + 250};
        drawtext(L"������δ���ţ������ڴ���", &tipRect, DT_CENTER | DT_VCENTER);
    }
}



// ���ƿ�ʼ����
void game_draw_start_screen() {
    // ���Ʊ���ͼƬ
    putimage(0, 0, &image_background);
    
    // ����������ʽ����ɫ
    settextstyle(40, 0, L"����");
    settextcolor(YELLOW);
    
    // ���Ʊ���Logo
    RECT logoRect = {0, height/4, width, height/4 + 60};
    drawtext(L"�ɻ���ս", &logoRect, DT_CENTER | DT_VCENTER);
    
    // ���Ʋ˵�ѡ��
    settextstyle(25, 0, L"����");
    settextcolor(WHITE);
    
    RECT option1Rect = {0, height/2 + 50, width, height/2 + 80};
    drawtext(L"��1�����޾�ģʽ", &option1Rect, DT_CENTER | DT_VCENTER);
    
    RECT option2Rect = {0, height/2 + 100, width, height/2 + 130};
    drawtext(L"��2���봳��ģʽ (δ����)", &option2Rect, DT_CENTER | DT_VCENTER);
    
    RECT option3Rect = {0, height/2 + 150, width, height/2 + 180};
    drawtext(L"��3����BOSSģʽ (δ����)", &option3Rect, DT_CENTER | DT_VCENTER);
    
    // �����Ҫ��ʾ��ʾ��Ϣ
    if (show_unavailable_msg) {
        settextcolor(RED);
        RECT tipRect = {0, height/2 + 220, width, height/2 + 250};
        drawtext(L"������δ���ţ������ڴ���", &tipRect, DT_CENTER | DT_VCENTER);
    }
}

// �ӵ��ƶ���������������ҷ�����ӵ���
void bullet_move(int speed)
{
	for (int i = 0; i < BULLET_NUM; i++)
	{
		// ֻ������ҷ�����ӵ���y���������ƶ���
		if (bullet[i].alive)  // �޸����Ƴ������y < player.y����
		{
			if(bullet_level>4&&bullet[i].type==0)
			{
				// �����νǶ��ƶ���x��y������λ��
				bullet[i].x += cos(bullet[i].angle) * speed;
				bullet[i].y -= sin(bullet[i].angle) * speed;
			}
			else{
				// ��ֱͨ���ƶ�
				bullet[i].y -= speed;
			}
			if (bullet[i].y < 0 || bullet[i].x < 0 || bullet[i].x > width)
			{
				bullet[i].alive = false;
			}
		}
	}
}

void attack_plane()//�ӵ�ײ���л�
{
	for (int i = 0; i < ENEMY_NUM; i++)
	{
		if (!enemy[i].alive)
		{
			continue;
		}
		if (enemy[i].alive)
		{
			for (int j = 0; j < BULLET_NUM; j++)
			{
				if (bullet[j].alive)
				{
					// �ڶ��ӵ���������ͨ��ײ��⣨���ڴ���ʱ������ел���
					if (bullet[j].type == 1)
						continue;
						
					if (bullet[j].y < enemy[i].y + enemy[i].height && bullet[j].y > enemy[i].y)
					{
						if (bullet[j].x > enemy[i].x && bullet[j].x < enemy[i].x + enemy[i].width)
						{
							enemy[i].hp--;
							bullet[j].alive = false;
							if (enemy[i].hp <= 0)
							{
								enemy[i].alive = false;
								enemy[i].fall = true;
								if (enemy[i].type==ENEMY_SMALL)
								{
									score += 10;
								}
								else if (enemy[i].type == ENEMY_MEDIUM)
								{
									score += 20;
								}
								else if (enemy[i].type == ENEMY_BIG)
								{
									score += 30;
								}
							}
						}
					}
				}
			}
		}
	}
}


void player_move(int speed)
{
	if (GetAsyncKeyState(0x57))  // W�� - ����
	{
		player.y -= speed;
		player.y = (player.y < 0) ? 0 : player.y;
	}
	if (GetAsyncKeyState(0x53))  // S�� - ����
	{
		player.y += speed;
		player.y = (player.y > height - player.height) ? height - player.height : player.y;
	}
	if (GetAsyncKeyState(0x41))  // A�� - ����
	{
		player.x -= speed;
		player.x = (player.x < 0) ? 0 : player.x;
	}
	if (GetAsyncKeyState(0x44))  // D�� - ����
	{
		player.x += speed;
		player.x = (player.x > width - player.width) ? width - player.width : player.x;
	}
	// ����ӵ�
	if (GetAsyncKeyState(0x4A))  // J�� - ����
	{
		if (is_blackhole_bullet)
		{
			// �ڶ��ӵ�������0.5��(500ms)��ʹ���µļ�ʱ��ID 5�����ͻ
			if (timer_arrive(5, 500))
			{
				creat_bullet();
			}
		}
		else
		{
			// ��ͨ�ӵ�����������0.1��(100ms)
			if (timer_arrive(3, 100))
			{
				creat_bullet();
			}
		}
	}
	// �س���������Ϸ
	if (GetAsyncKeyState(VK_RETURN) && restart)
	{
		restart = false;
		game_init(); // ���³�ʼ����Ϸ״̬
		is_blackhole_bullet = false; // ���úڶ��ӵ�״̬
	}

    // ����޵�ʱ���Ƿ����
    if (is_invincible && clock() > invincible_end_time) {
        is_invincible = false;
    }
}

void game_draw()
{


	putimage(0, 0, &image_background);//���Ʊ���
	//������
	RECT scoreRect = { 0, 0, width, height };
	setbkmode(TRANSPARENT);
	settextstyle(25, 15, L"����");
	settextcolor(WHITE);
	
	// ������߷֣���ࣩ
	std::wstring highScoreText = L"��߷֣�" + std::to_wstring(high_score);
	drawtext(highScoreText.c_str(), &scoreRect, DT_TOP | DT_LEFT | DT_CENTER);
	
	// ���Ƶ�ǰ�������Ҳࣩ
	std::wstring scoreText = L"������" + std::to_wstring(score);
	RECT scoreRectRight = { 0, 30, width, height }; // ����30���ر����ص�
	drawtext(scoreText.c_str(), &scoreRectRight, DT_TOP | DT_LEFT | DT_CENTER);

	// ��ӵȼ���ʾ������λ�ã�
	RECT levelRect = {20, height/2 - 15, width, height}; // ����λ��
	settextstyle(25, 15, L"����");
	settextcolor(YELLOW); // ��ɫ����ͻ����ʾ
	std::wstring levelText = L"�ȼ���" + std::to_wstring(bullet_level);
	drawtext(levelText.c_str(), &levelRect, DT_TOP | DT_LEFT);

	// ���µ����ȼ���ÿ200������1����
	int new_bullet_level = 1 + (score / 200);
	if (new_bullet_level > bullet_level) {
		bullet_level = new_bullet_level;
	}
	if (player.alive)
	{
		drawAlpha(&image_player[0], player.x, player.y);
	}
	if (player.fall)
	{
		// ʹ��Ψһ��ʱ��ID 6��������ڶ��ӵ������ʱ����ͻ
		if (timer_arrive(6, 200)) // ÿ200�����л�һ֡
		{
			player_fall_cnt++;

			// ����������֡��������Ϸ����
			if (player_fall_cnt >= 4)
			{
				player.fall = false;
				player_fall_cnt = 0;  // ȷ������������
				restart = true; // ��ʾ������ť
			}
		}

		// ʼ�ջ��Ƶ�ǰ֡
		if (player_fall_cnt < 4)
		{
			drawAlpha(&image_player[player_fall_cnt + 1], player.x, player.y);
		}
	}
	//���ƿ�Ͷ
	for (int i = 0; i < AIRDROP_NUM; i++)
	{
		if (airdrops[i].alive)
		{
			drawAlpha(&image_airdrop[airdrops[i].type], airdrops[i].x, airdrops[i].y);
		}
	}
	//���Ƶл�
	for (int i = 0; i < ENEMY_NUM; i++)
	{
		// ֻ�з�BOSSģʽ�Ż�����ͨ�л�
		if (!is_boss_mode && enemy[i].alive)
		{
			if (enemy[i].type == ENEMY_SMALL)
			{
				drawAlpha(&image_enemy[0], enemy[i].x, enemy[i].y);
			}
			//���͵л�
			if (enemy[i].type == ENEMY_MEDIUM)
			{
				drawAlpha(&image_enemy[1], enemy[i].x, enemy[i].y);

			}
			//���͵л�
			if (enemy[i].type == ENEMY_BIG)
			{
				drawAlpha(&image_enemy[2], enemy[i].x, enemy[i].y);
			}
		}
		//���Ƶл������ж���
		if (enemy[i].fall)
		{
			if (timer_arrive(7 + i, 150))
			{
				enemy_fall_cnt[i]++;
			}
			if (enemy[i].type == 0)
			{
				if (enemy_fall_cnt[i] < 4)
				{
					drawAlpha(&image_enemy_fall_1[enemy_fall_cnt[i]], enemy[i].x, enemy[i].y);
				}
			}
			if (enemy[i].type == 1)
			{
				if (enemy_fall_cnt[i] < 4)
				{
					drawAlpha(&image_enemy_fall_2[enemy_fall_cnt[i]], enemy[i].x, enemy[i].y);
				}
			}
			if (enemy[i].type == 2)
			{
				if (enemy_fall_cnt[i] < 6)
				{
					drawAlpha(&image_enemy_fall_3[enemy_fall_cnt[i]], enemy[i].x, enemy[i].y);
				}
			}
			if (enemy[i].type == 2 && enemy_fall_cnt[i] >= 5)
			{
				enemy[i].fall = false;
				enemy_fall_cnt[i] = 0;
			}
			if (enemy[i].type == 0 && enemy[i].type == 1 && enemy_fall_cnt[i] >= 4)
			{
				enemy[i].fall = false;
				enemy_fall_cnt[i] = 0;
			}
		}	
	}
	
	// �������Ѫ��
    RECT hpRect = {20, 70, 200, 100};  // λ���ڷ����·�
    settextstyle(20, 10, L"����");
    settextcolor(RED);
    wchar_t hpText[50];
    swprintf_s(hpText, L"Ѫ��: %d", player.hp);
    drawtext(hpText, &hpRect, DT_LEFT | DT_VCENTER);

	// ����BOSS
	if (is_boss_mode)
	{
		if (boss1.alive)
		{
			drawAlpha(&image_boss1[0], boss1.x, boss1.y);
			
			// ����BOSSѪ��
			setfillcolor(RED);
			fillrectangle(boss1.x, boss1.y - 10, boss1.x + boss1.width * (boss1.hp / (float)boss1.max_hp), boss1.y - 5);
			setlinecolor(WHITE);
			rectangle(boss1.x, boss1.y - 10, boss1.x + boss1.width, boss1.y - 5);
		}
		else if (boss_defeated)
		{
			// ��������֡�л��ٶȣ���150ms��Ϊ200ms
			if (timer_arrive(22, 200))
			{
				boss_explosion_frame++;
			}
			
			// ������ʾ��ը֡
			if (boss_explosion_frame == 0)
			{
				drawAlpha(&image_boss1[1], boss1.x, boss1.y);
			}
			else if (boss_explosion_frame == 1)
			{
				drawAlpha(&image_boss1[2], boss1.x, boss1.y);
			}
			else if (boss_explosion_frame >= 2)
			{
				drawAlpha(&image_boss1[3], boss1.x, boss1.y);
			}
		}
	}

	//�����ؿ�����
	if (restart)
	{
		drawAlpha(&image_restart, width/5,height/3);
		for (int i = 0; i < BULLET_NUM; i++)
		{
			bullet[i].alive = false;
		}
		for (int i = 0; i < ENEMY_NUM; i++)
		{
			enemy[i].alive = false;
		}
	}

	//�����ӵ�
	for (int i = 0; i < BULLET_NUM; i++)
	{
		if (bullet[i].alive)
		{
			// �����ӵ����ͻ��ƣ�����ȫ��״̬
			if (bullet[i].type == 1)
			{
				drawAlpha(&image_blackhole_bullet, bullet[i].x, bullet[i].y);
			}
			else
			{
				drawAlpha(&image_bullet, bullet[i].x, bullet[i].y);
			}
		}
	}
	// ���ƻ���״̬������λ����ʾ��
    if (has_shield) {
        // ���ƻ���ͼ�꣨ʹ���Ѽ��ص�image_airdrop[1]��
        drawAlpha(&image_airdrop[1], 20, 50);  // ����ɸ���ʵ�ʵ���
        
        // ��������˵��
        RECT shieldTextRect = { 60, 50, 200, 80 };  // ��������ͼ���Ҳࣩ
        settextstyle(20, 10, L"����");  // ���������С
        settextcolor(WHITE);            // ��ɫ����
        drawtext(L"���ܼ���", &shieldTextRect, DT_VCENTER | DT_LEFT);  // ��ֱ����+�����
    }

    // �����޵�״̬������λ����ʾ��
    if (is_invincible) {
        // ����ʣ��ʱ�䣨�룩
        int remaining = max(0, (int)(invincible_end_time - clock()) / 1000);
        
        // ���ƻ���ͼ�꣨ʹ���Ѽ��ص�image_airdrop[1]��
        drawAlpha(&image_airdrop[1], 20, 50);  // ����ɸ���ʵ�ʵ���
        
        // ��������˵������ʾʣ��ʱ�䣩
        RECT invincibleTextRect = { 60, 50, 200, 80 };  // ��������ͼ���Ҳࣩ
        settextstyle(20, 10, L"����");  // ���������С
        settextcolor(WHITE);            // ��ɫ����
        wchar_t text[50];
        swprintf_s(text, L"�޵�ʣ�ࣺ%d��", remaining);
        drawtext(text, &invincibleTextRect, DT_VCENTER | DT_LEFT);  // ��ֱ����+�����
    }
}
// ... existing code ...
//���ɵл���Ѫ��������
void enemy_hp(int index)
{
    // ֻ�е��÷ֳ���600��ſ�ʼ����Ѫ���ӳɣ�ÿ100������һ�Σ�
    int score_bonus = (score > 600) ? ((score - 600) / 100) : 0; // �÷ֳ���600��ÿ100������һ�μӳɻ���
    
    // ������ɵл�����
    int random_type = rand() % 20; // ��������Է�Χ
    
    if (random_type == 0) {
        // ���͵л�
        enemy[index].type = ENEMY_BIG;
        // ����Ѫ��3 + �÷ֳ���600��ÿ100������15��Ѫ
        enemy[index].hp = 3 + (score_bonus * 6);
        enemy[index].height = 258;
        enemy[index].width = 169;
    }
    // ���͵л�
    else if (random_type == 2 || random_type == 4) {
        enemy[index].type = ENEMY_MEDIUM;
        // ����Ѫ��2 + �÷ֳ���600��ÿ100������10��Ѫ
        enemy[index].hp = 2 + (score_bonus * 4);
        enemy[index].height = 99;
        enemy[index].width = 69;
    }
    // С�͵л�
    else {
        enemy[index].type = ENEMY_SMALL;
        // ����Ѫ��1 + �÷ֳ���600��ÿ100������5��Ѫ
        enemy[index].hp = 1 + (score_bonus * 2);
        enemy[index].height = 43;
        enemy[index].width = 57;
    }
}
// ... existing code ...
//������ɵл���λ��
void creat_enemy()
{
	for (int i = 0; i < ENEMY_NUM; i++)
	{
		if (!enemy[i].alive)
		{
			//enemy[i].type = ENEMY_SMALL;
			enemy_hp(i);
			enemy[i].x = rand() % (width - enemy[i].width);
			enemy[i].y = rand() % 60;
			enemy[i].alive = true;
			enemy[i].fall = false;
			//������������
			//fall_cnt[i] = 0;
			break;
		}
	}
}

//�л��ƶ�
void enemy_move(int speed)
{
	for (int i = 0; i < ENEMY_NUM; i++)
	{
		if (enemy[i].alive)
		{
			enemy[i].y += speed;
			if (enemy[i].y > height)
			{
				enemy[i].alive = false;
				//enemy[i].fall = true; 
			}
		}
	}
}

// BOSS�ƶ�
void boss_move(float speed)  // �޸Ĳ���Ϊfloat������֧��С���ٶ�
{
	if (!boss1.alive) return;
	
	// ʹ�ø������ۻ�λ��ʵ�������ƶ�
	static float move_offset = 0.0f;
	move_offset += speed;
	
	// ���ۻ�λ�ƴﵽ1����ʱ��ʵ���ƶ�
	if (move_offset >= 1.0f || move_offset <= -1.0f)
	{
		boss1.x += boss1.move_direction * (int)move_offset;
		move_offset -= (int)move_offset;
	}
	
	// �߽��⣬�����߽�ı䷽��
	if (boss1.x <= 0)
		boss1.move_direction = 1;
	if (boss1.x + boss1.width >= width)
		boss1.move_direction = -1;
}

// ���BOSS���ӵ���ײ���Ż���ײ��⣩
void check_boss_collision()
{
	if (!boss1.alive) return;
	
	for (int j = 0; j < BULLET_NUM; j++)
	{
		if (bullet[j].alive && bullet[j].type == 0) // ֻ������ͨ�ӵ��˺�BOSS
		{
			// ��ȷ��ײ��⣨�����ӵ�ȫ������
			if (bullet[j].x < boss1.x + boss1.width &&
				bullet[j].x + bullet[j].width > boss1.x &&
				bullet[j].y < boss1.y + boss1.height &&
				bullet[j].y + bullet[j].height > boss1.y)
			{
				boss1.hp--;
				bullet[j].alive = false;  // ���������ӵ�
				
				if (boss1.hp <= 0)
				{
					boss1.alive = false;
					boss_defeated = true;
					boss_defeat_time = clock();
					boss_explosion_frame = 0; 
					score += 30; 
				}
			}
		}
	}
}

// ... existing code ...
// ���л��������ײ
void check_enemy_collision() {
    if (!player.alive || is_invincible) return; // ������������޵�״̬�������ײ
    
    for (int i = 0; i < ENEMY_NUM; i++) {
        if (enemy[i].alive) {
            // ���л�����ҵľ�����ײ
            if (player.x < enemy[i].x + enemy[i].width &&
                player.x + player.width > enemy[i].x &&
                player.y < enemy[i].y + enemy[i].height &&
                player.y + player.height > enemy[i].y) {
                
                // ��ײ����
                enemy[i].alive = false;  // �л���ʧ
                enemy[i].fall = true;   // �����л���ը����
                
                // �۳����1��Ѫ��
                player.hp -= 1;
                
                // ��Ӷ����޵�ʱ���ֹ������Ѫ
                is_invincible = true;
                invincible_end_time = clock() + 1000; // �޵�״̬����1��
                
                // �������Ƿ�Ѫ���ľ�
                if (player.hp <= 0) {
                    player.alive = false;
                    player.fall = true;  // ������ұ�ը����
                }
            }
        }
    }
}
// ... existing code ...

// ���ɿ�Ͷ
void creat_airdrop() {
    

    for (int i = 0; i < AIRDROP_NUM; i++) {
        if (!airdrops[i].alive) {
            // ������ɿ�Ͷ(1/30����)
            if (rand() % 30 == 0) {
                airdrops[i].x = rand() % (width - 50);
                airdrops[i].y = -50; // ����Ļ���������
                airdrops[i].width = 50;
                airdrops[i].height = 50;
                airdrops[i].alive = true;
                // ���ѡ���Ͷ����: 0-�ڶ��ӵ�, 1-����,2-����,3-HP
                airdrops[i].type = rand() % 4; 
                break;
            }
        }
    }
}

// ��Ͷ�ƶ�
void airdrop_move(int speed) {
    for (int i = 0; i < AIRDROP_NUM; i++) {
        if (airdrops[i].alive) {
            airdrops[i].y += speed;
            // ���������Ļ�ײ�������Ϊ����Ծ
            if (airdrops[i].y > height) {
                airdrops[i].alive = false;
            }
        }
    }
}

// ����Ͷ��ײ
// ����Ͷ��ײ
void check_airdrop_collision() {
    if (!player.alive) return;

    for (int i = 0; i < AIRDROP_NUM; i++) {
        if (airdrops[i].alive) {
            // �����ײ
            if (player.x < airdrops[i].x + airdrops[i].width &&
                player.x + player.width > airdrops[i].x &&
                player.y < airdrops[i].y + airdrops[i].height &&
                player.y + player.height > airdrops[i].y) {
                
                // ���ݿ�Ͷ����Ӧ�ò�ͬЧ��
                if (airdrops[i].type == 0) {
                    // �ڶ��ӵ�Ч��
                    is_blackhole_bullet = true;
                    blackhole_bullet_end_time = clock() + 5000;
                } 
                else if (airdrops[i].type == 1) {
                    // �޸�Ϊ8���޵�Ч��
                    is_invincible = true;
                    invincible_end_time = clock() + 8000;  // 8000ms=8��
                }
                // ��Ӿ����ͶЧ��
                else if (airdrops[i].type == 2) {
                    score += 50;  // ��50��
                }else if (airdrops[i].type == 3) {
                    player.hp+=1;  // ��Ѫ
					// ��ʾ��Ѫ��ʾ
                    RECT healthTipRect = {width/2 - 100, height/2, width/2 + 100, height/2 + 30};
                    settextstyle(20, 10, L"����");
                    settextcolor(GREEN);
                    drawtext(L"Ѫ��+1!", &healthTipRect, DT_CENTER | DT_VCENTER);
                }
                
                airdrops[i].alive = false; // ʰȡ����ʧ
            }
        }
    }
}

void load_image()
{
	//���ر���ͼƬ
	loadimage(&image_player[0], _T("./images/me.png"), player.width, player.height);
	loadimage(&image_player[1], _T("./images/me_destroy_1.png"), player.width, player.height);
	loadimage(&image_player[2], _T("./images/me_destroy_2.png"), player.width, player.height);
	loadimage(&image_player[3], _T("./images/me_destroy_3.png"), player.width, player.height);
	loadimage(&image_player[4], _T("./images/me_destroy_4.png"), player.width, player.height);
	//���ر���ͼƬ
	loadimage(&image_background, _T("./images/background.png"), width, height);
	//���صл�ͼƬ
	loadimage(&image_enemy[0], _T("./images/enemy1.png"));
	loadimage(&image_enemy[1], _T("./images/enemy2.png"));
	loadimage(&image_enemy[2], _T("./images/enemy3.png"));

	loadimage(&image_restart, _T("./images/again.png"));//����������ťͼƬ

	loadimage(&image_bullet, _T("./images/bullet.png"));//�����ӵ�
	// ������һ������true��32λɫ����أ�����Alphaͨ��
	loadimage(&image_blackhole_bullet, _T("./images/blackhole_bullet.png"), 40, 40, true);//���غڶ��ӵ�

	loadimage(&image_enemy_fall_1[0], _T("./images/enemy1_down1.png"));//�������ֲ�ͬ�л�������Ч��
	loadimage(&image_enemy_fall_1[1], _T("./images/enemy1_down2.png"));
	loadimage(&image_enemy_fall_1[2], _T("./images/enemy1_down3.png"));
	loadimage(&image_enemy_fall_1[3], _T("./images/enemy1_down4.png"));

	loadimage(&image_enemy_fall_2[0], _T("./images/enemy2_down1.png"));
	loadimage(&image_enemy_fall_2[1], _T("./images/enemy2_down2.png"));
	loadimage(&image_enemy_fall_2[2], _T("./images/enemy2_down3.png"));
	loadimage(&image_enemy_fall_2[3], _T("./images/enemy2_down4.png"));

	loadimage(&image_enemy_fall_3[0], _T("./images/enemy3_down1.png"));
	loadimage(&image_enemy_fall_3[1], _T("./images/enemy3_down2.png"));
	loadimage(&image_enemy_fall_3[2], _T("./images/enemy3_down3.png"));
	loadimage(&image_enemy_fall_3[3], _T("./images/enemy3_down4.png"));
	loadimage(&image_enemy_fall_3[4], _T("./images/enemy3_down5.png"));
	loadimage(&image_enemy_fall_3[5], _T("./images/enemy3_down6.png"));
	//���ؿ�ͶͼƬ�����true��������Alphaͨ��
	loadimage(&image_airdrop[0], _T("./images/blackhole_airdrop.png"), 90, 90, true);
	loadimage(&image_airdrop[1], _T("./images/shields.png"), 60, 60, true);
	loadimage(&image_airdrop[2], _T("./images/experience.png"), 90, 90, true);
	loadimage(&image_airdrop[3], _T("./images/HP.png"), 60, 60, true);
	// ����BOSS1ͼƬ
	loadimage(&image_boss1[0], _T("./images/boss1.png"), 150, 300, true);
	loadimage(&image_boss1[1], _T("./images/boss1_fall_1.png"), 150, 300, true);
	loadimage(&image_boss1[2], _T("./images/boss1_fall_2.png"), 150, 300, true);
	loadimage(&image_boss1[3], _T("./images/boss1_fall_3.png"), 200, 200, true);
	//����boss2ͼƬ
	loadimage(&image_boss2[0], _T("./images/boss2_1.png"), 150, 300, true);
	loadimage(&image_boss2[1], _T("./images/boss2_2.png"), 150, 300, true);
	loadimage(&image_boss2[2], _T("./images/boss2_3.png"), 150, 300, true);
	loadimage(&image_boss2[3], _T("./images/boss2_4.png"), 150, 300, true);
	loadimage(&image_boss2[4], _T("./images/boss2_5.png"), 200, 200, true);
	
}


void player_death()//��������
{
    for (int i = 0; i < ENEMY_NUM; i++) {
        if (player.x + player.width < enemy[i].x || player.x > enemy[i].x + enemy[i].width || player.y + player.height < enemy[i].y || player.y > enemy[i].y + enemy[i].height) {
            continue;
        } else {
            if (is_invincible) {
                // �޵��ڼ䲻���������������ٵл�
                enemy[i].alive = false;
            } else {
				// player.hp-=1;
				// if (player.hp <= 0) {
                // player.alive = false;
                // player.fall = true;
                // enemy[i].alive = false;
				check_enemy_collision();
				}
                // ���²�������߷�
                if (score > high_score)
                {
                    high_score = score;
                    // ������߷ֵ��ļ�
                    ofstream outfile(HIGH_SCORE_FILE, ios::binary | ios::trunc);
                    if (outfile.is_open())
                    {
                        outfile.write(reinterpret_cast<const char*>(&high_score), sizeof(high_score));
                        outfile.close();
                    }
                }
            }
        }
 }


// ΪBOSS�ӵ���ӵ������ƶ�����
void boss_bullet_move(int speed)
{
	for (int i = 0; i < BULLET_NUM; i++)
	{
		// ����Ƿ���BOSS������ӵ���y���������ƶ���
		if (bullet[i].alive && bullet[i].y > player.y)
		{
			bullet[i].y += speed; // BOSS�ӵ������ƶ�
			if (bullet[i].y > height)
			{
				bullet[i].alive = false;
			}
		}
	}
}

// �޸�BOSSս���º���
void update_boss_battle(int speed)
{
	// ��Ҳ���
	if (timer_arrive(0, 10))
		player_move(speed);
	if (timer_arrive(1, 10))
		bullet_move(3); // ����ӵ������ƶ�
		
	// BOSS��Ϊ
	if (boss1.alive)
	{
		boss_move(0.5f);  // ����BOSS�ƶ��ٶ�Ϊ0.5
		check_boss_collision();
	}
	// BOSS�����ܺ���������
	else if (boss_defeated)
	{
		// ÿ200ms�л�һ֡����
		if (timer_arrive(22, 200)) {
			boss_explosion_frame++;
		}

		// ���������ж���֡���л�ģʽ������boss_defeatedΪtrue��
		if (boss_explosion_frame >= 3) 
		{
			is_boss_mode = false;  // �л�����ͨģʽ
			// ����������boss_defeated�������������´���BOSSս
			// boss_defeated = false;
			boss_explosion_frame = 0;
			boss1.hp = boss1.max_hp;
			boss1.x = width / 2 - boss1.width / 2;
			boss1.y = 50;
			boss1.alive = false;
		}
	}
}

// ����ͨ��Ϸ���º��������boss_defeated�����߼�
void update_normal_game(int speed)
{
	if (timer_arrive(0, 10))
		player_move(speed);
	if (timer_arrive(1, 10))
		bullet_move(3);
	if (timer_arrive(2, 1000))
		creat_enemy();
	if (timer_arrive(4, 50))
		enemy_move(4);
	if (timer_arrive(17, 3000))
		creat_airdrop();
	if (timer_arrive(18, 50))
		airdrop_move(2);
	check_airdrop_collision();
}

// ��main������ʹ���µķ��뺯��
int main()
{
	srand((unsigned)time(NULL));//
	loadimage(&image_background, L"./images/background.png");
	int speed = 5; //�����ٶ�
	game_init(); //��ʼ����Ϸ
	load_image();//��������ͼƬ
	BeginBatchDraw();
	while (1)
	{
		if (current_state == START_SCREEN) {
            // ���ƿ�ʼ����
            draw_start_screen();
			FlushBatchDraw();
            // �����������
            if (GetAsyncKeyState(0x31)) { // ��1��
                current_state = INFINITE_MODE;
                game_init(); // ��ʼ����Ϸ״̬
            } else if (GetAsyncKeyState(0x32) || GetAsyncKeyState(0x33)) { // ��2��3��
                show_unavailable_msg = true;
                msg_show_time = GetTickCount();
            }
        }else{
		game_draw();
		FlushBatchDraw();
		// ֻ�е���Ϸδ��������״̬ʱ����ִ����Ϸ�߼�
		if (!restart)
		{
			// �������Ƿ�ﵽ100�֣�����BOSSս - �����ȷ�Ĵ�����������
			const int BOSS_TRIGGER_SCORE = 100;
			if (score >= BOSS_TRIGGER_SCORE && !is_boss_mode && !boss_defeated)
			{
				is_boss_mode = true;
				boss1.alive = true;
				boss1.hp = boss1.max_hp; // ����BOSSѪ��
				
				// ��յ�ǰ�л��Ϳ�Ͷ
				for (int i = 0; i < ENEMY_NUM; i++)
				{
					enemy[i].alive = false;
				}
				for (int i = 0; i < AIRDROP_NUM; i++)
				{
					airdrops[i].alive = false;
				}
			}
			
			// ������Ϸģʽ���ò�ͬ�ĸ��º���
			if (is_boss_mode)
			{
				update_boss_battle(speed);
			}
			else
			{
				// ����ͨģʽ������boss_defeated״̬�������´δﵽ����ʱ�ٴδ���BOSSս
				// if (boss_defeated)
				// {
				// 	boss_defeated = false;
				// }
				update_normal_game(speed);
			}
			
			// ���ڶ��ӵ��Ƿ����
			if (is_blackhole_bullet && clock() > blackhole_bullet_end_time)
			{
				is_blackhole_bullet = false;
				// ������л�Ծ�ĺڶ��ӵ�
				for (int i = 0; i < BULLET_NUM; i++)
				{
					if (bullet[i].alive && bullet[i].type == 1)
					{
						bullet[i].alive = false;
					}
				}
			}
			attack_plane(); // �ӵ������л�
			player_death(); // �����ҷɻ��Ƿ�����
		}
		else
		{
			// ��������������
			if (GetAsyncKeyState(VK_RETURN))
			{
				restart = false;
				game_init(); // ���³�ʼ����Ϸ״̬
				is_blackhole_bullet = false; // ���úڶ��ӵ�״̬
				is_boss_mode = false;
				boss_defeated = false;
			}
		}
	}
}
	EndBatchDraw();
	return 0;
}

// ͼƬ͸����������ü��ɣ����Դ���װ����easyx�������޷�͸����
void drawAlpha(IMAGE* picture, int picture_x, int picture_y) {
	DWORD* dst = GetImageBuffer(); // ��ȡĿ���ͼ��������
	DWORD* src = GetImageBuffer(picture); // ��ȡԴͼƬ���������ؼ�������

	int picture_width = picture->getwidth();
	int picture_height = picture->getheight();
	int graphWidth = getwidth();
	int graphHeight = getheight();

	for (int iy = 0; iy < picture_height; iy++) {
		for (int ix = 0; ix < picture_width; ix++) {
			// ����Դͼ������λ��
			int srcX = ix + iy * picture_width;

			// ����Ŀ��ͼ��λ�ò����߽�
			int dstX = (ix + picture_x) + (iy + picture_y) * graphWidth;
			if (ix + picture_x < 0 || ix + picture_x >= graphWidth ||
				iy + picture_y < 0 || iy + picture_y >= graphHeight) {
				continue; // ����Խ������
			}

			// ��ȡalphaͨ��ֵ
			int sa = ((src[srcX] & 0xff000000) >> 24);
			if (sa == 0) continue; // ��ȫ͸��������������

			// ��ȡRGB����
			int sr = ((src[srcX] & 0xff0000) >> 16);
			int sg = ((src[srcX] & 0xff00) >> 8);
			int sb = src[srcX] & 0xff;

			// ��ȡĿ��������ɫ
			int dr = ((dst[dstX] & 0xff0000) >> 16);
			int dg = ((dst[dstX] & 0xff00) >> 8);
			int db = dst[dstX] & 0xff;

			// ִ��alpha���
			dst[dstX] = (
				((sr * sa + dr * (255 - sa)) / 255) << 16 |
				((sg * sa + dg * (255 - sa)) / 255) << 8 |
				((sb * sa + db * (255 - sa)) / 255)
				);
		}
	}
}

bool timer_arrive(int id, int ms)//������ȴʱ�䣬��ֹ�ɻ��ƶ����죬��ֹ���ɹ��죬�л�����1���ӵ���ȴ2���л��ƶ�3����������4
{
	static vector<DWORD> t(100);
	if (clock() - t[id] > ms)
	{
		t[id] = clock();
		return true;
	}
	return false;
}

void game_run() {
    while (true) {
        if (current_state == START_SCREEN) {
            // ���ƿ�ʼ����
            draw_start_screen();
            
            // �����������
            if (GetAsyncKeyState(0x31)) { // ��1�� - �����޾�ģʽ
                current_state = INFINITE_MODE;
                game_init(); // ��ʼ����Ϸ״̬
            } else if (GetAsyncKeyState(0x32) || GetAsyncKeyState(0x33)) { // ��2��3��
                show_unavailable_msg = true;
                msg_show_time = GetTickCount();
            }
            
            // 3�������"��δ����"��ʾ
            if (show_unavailable_msg && GetTickCount() - msg_show_time > 3000) {
                show_unavailable_msg = false;
            }
            
            Sleep(50);
            continue;
        }else{
			 // ��Ϸ�߼�����
			 player_move(5);
			 creat_enemy();
			 enemy_move(3);
			 bullet_move(10);
			 attack_plane();
			 check_airdrop_collision();
			 check_enemy_collision(); // ��ӵл��������ײ���
			 game_draw();
		}
        
        // ... existing game logic ...
    }
}



