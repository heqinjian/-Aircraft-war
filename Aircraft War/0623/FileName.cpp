#include<easyx.h>
#include<vector>
#include<string>
#include<time.h>
using namespace std;


#define BULLET_NUM 15 //允许的最大子弹数
#define ENEMY_NUM 10 //允许的最大敌机数
#define ENEMY_SMALL	  0 //小型敌机
//中型敌机
#define ENEMY_MEDIUM 1 
//大型敌机
#define ENEMY_BIG    2
#define AIRDROP_NUM 5 //空投数量
bool is_blackhole_bullet = false; // 是否使用黑洞子弹
DWORD blackhole_bullet_end_time = 0; // 黑洞子弹结束时间
bool is_invincible = false; // 新增：是否处于无敌状态
DWORD invincible_end_time = 0; // 新增：无敌结束时间
bool has_shield = false; // 是否拥有护盾
bool is_boss_mode = false; // BOSS战模式标志
bool boss_defeated = false; // BOSS是否被击败
DWORD boss_defeat_time = 0; // BOSS被击败的时间
int boss_explosion_frame = 0; // BOSS爆炸动画帧计数器
struct Boss {
    int x;
    int y;
    int width;
    int height;
    int hp;
    int max_hp;
    bool alive;
    int move_direction; // 1: 右移, -1: 左移
} boss1;

struct Airdrop {
    int x;
    int y;
    int width;
    int height;
    bool alive;
    int type; // 0: 黑洞子弹
};
bool restart=false;
enum window
{
	width = 480,
	height = 600,
};
int score = 0; // 积分
// 游戏是否重启
static int player_fall_cnt = 0; //本机死亡动画计数
static vector<int> enemy_fall_cnt(ENEMY_NUM); //敌机死亡动画计数
//敌机死亡步骤
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
}player;

vector<IMAGE> image_player(5);//本机结构体数组（正常+4帧爆炸）
vector<IMAGE> image_airdrop(AIRDROP_NUM);//空投
vector<plane> enemy(ENEMY_NUM);//敌机结构体数组
vector<IMAGE> image_enemy(5);//敌机照片（小型+中型+大型）
// 敌机被击中图片（小型）
vector<IMAGE> image_enemy_fall_1(4);
// 敌机被击中图片（中型）
vector<IMAGE> image_enemy_fall_2(4);
// 敌机被击中图片（大型）
vector<IMAGE> image_enemy_fall_3(6);
vector<IMAGE> image_boss1(4); // BOSS图片
vector<plane>bullet(BULLET_NUM);//子弹结构体数组
IMAGE image_bullet;// 子弹图片
IMAGE image_blackhole_bullet;// 黑洞子弹图片

vector<Airdrop> airdrops(AIRDROP_NUM); // 空投结构体数组
IMAGE image_background;//背景图片

// 重启按钮图片
IMAGE image_restart;
void drawAlpha(IMAGE* picture, int picture_x, int picture_y);
bool timer_arrive(int id, int ms);

void game_init()
{
	//初始化玩家飞机
	player.width = 80;
	player.height = 80;
	player.x = width / 2 - player.width / 2;
	player.y = height - player.height;
	player.alive = true;
	player.fall = false;
	score = 0; // 分数归零
	
	// 初始化BOSS
	boss1.width = 150;
	boss1.height = 100;
	boss1.x = width / 2 - boss1.width / 2;
	boss1.y = 50;
	boss1.hp = 66;
	boss1.max_hp = 66;
	boss1.alive = false;
	boss1.move_direction = 1; // 初始向右移动
	
	//初始化敌机
	int i = 0;
	for (i = 0; i < ENEMY_NUM; i++)
	{
		enemy[i].alive = false;
	}
	//初始化子弹
	for (int i = 0; i < BULLET_NUM; i++)
	{
		bullet[i].alive = false;
		bullet[i].width = 5;
		bullet[i].height = 15;
	}
	
	// 初始化空投
	for (int i = 0; i < AIRDROP_NUM; i++) {
		airdrops[i].alive = false;
		airdrops[i].type = 0; // 默认黑洞子弹类型
	}

	initgraph(width, height); //初始化窗口
}


//生成子弹函数
void creat_bullet()
{
	for (int i = 0; i < BULLET_NUM; i++)
	{
		if (!bullet[i].alive)
		{
			bullet[i].x = player.x + player.width / 2;
			bullet[i].y = player.y - 5;
			bullet[i].alive = true;
			// 标记子弹类型：1为黑洞子弹，0为普通子弹
			bullet[i].type = is_blackhole_bullet ? 1 : 0;
			
			// 黑洞子弹特殊效果：清空屏幕所有敌机
			if (bullet[i].type == 1)
			{
				for (int j = 0; j < ENEMY_NUM; j++)
				{
					if (enemy[j].alive)
					{
						enemy[j].alive = false;
						enemy[j].fall = true;
						// 根据敌机类型增加分数
						if (enemy[j].type == ENEMY_SMALL)
							score += 1;
						else if (enemy[j].type == ENEMY_MEDIUM)
							score += 2;
						else if (enemy[j].type == ENEMY_BIG)
							score += 3;
					}
				}
			}
			break;
		}
	}
}
// 子弹移动函数（仅处理玩家发射的子弹）
void bullet_move(int speed)
{
	for (int i = 0; i < BULLET_NUM; i++)
	{
		// 只处理玩家发射的子弹（y方向向上移动）
		if (bullet[i].alive)  // 修复：移除错误的y < player.y条件
		{
			bullet[i].y -= speed;
			if (bullet[i].y < 0)
			{
				bullet[i].alive = false;
			}
		}
	}
}

void attack_plane()//子弹撞击敌机
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
					// 黑洞子弹不参与普通碰撞检测（已在创建时清除所有敌机）
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
									score += 1;
								}
								else if (enemy[i].type == ENEMY_MEDIUM)
								{
									score += 2;
								}
								else if (enemy[i].type == ENEMY_BIG)
								{
									score += 3;
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
	if (GetAsyncKeyState(VK_UP))
	{
		player.y -= speed;
		player.y = (player.y < 0) ? 0 : player.y;
	}
	if (GetAsyncKeyState(VK_DOWN))
	{
		player.y += speed;
		player.y = (player.y >= height - player.height) ? height - player.height : player.y;
	}
	if (GetAsyncKeyState(VK_LEFT))
	{
		player.x -= speed;
		player.x = (player.x < 0 - player.width / 2) ? 0 - player.width / 2 : player.x;
	}
	if (GetAsyncKeyState(VK_RIGHT))
	{
		player.x += speed;
		player.x = (player.x > width - player.width / 2) ? width - player.width / 2 : player.x;
	}
	// 射击子弹
	if (GetAsyncKeyState(VK_SPACE))
	{
		if (is_blackhole_bullet)
		{
			// 黑洞子弹射击间隔0.5秒(500ms)，使用新的计时器ID 5避免冲突
			if (timer_arrive(5, 500))
			{
				creat_bullet();
			}
		}
		else
		{
			// 普通子弹射击间隔保持0.1秒(100ms)
			if (timer_arrive(3, 100))
			{
				creat_bullet();
			}
		}
	}
	// 回车键重启游戏
	if (GetAsyncKeyState(VK_RETURN) && restart)
	{
		restart = false;
		game_init(); // 重新初始化游戏状态
		is_blackhole_bullet = false; // 重置黑洞子弹状态
	}

    // 检测无敌时间是否过期
    if (is_invincible && clock() > invincible_end_time) {
        is_invincible = false;
    }
}

void game_draw()
{


	putimage(0, 0, &image_background);//绘制背景
	//贴文字
	RECT rect = { 0, 0, width, height };
	setbkmode(TRANSPARENT);//文字背景透明
	// 使用wstring避免字符集问题
	std::wstring text = L"分数：" + std::to_wstring(score);
	settextstyle(25, 15, L"黑体");
	drawtext(text.c_str(), &rect, DT_TOP | DT_CENTER);//绘制文字置顶且居中
	if (player.alive)
	{
		drawAlpha(&image_player[0], player.x, player.y);
	}
	if (player.fall)
	{
		// 使用唯一计时器ID 6，避免与黑洞子弹射击计时器冲突
		if (timer_arrive(6, 200)) // 每200毫秒切换一帧
		{
			player_fall_cnt++;

			// 播放完所有帧后设置游戏结束
			if (player_fall_cnt >= 4)
			{
				player.fall = false;
				player_fall_cnt = 0;  // 确保计数器重置
				restart = true; // 显示重启按钮
			}
		}

		// 始终绘制当前帧
		if (player_fall_cnt < 4)
		{
			drawAlpha(&image_player[player_fall_cnt + 1], player.x, player.y);
		}
	}
	//绘制空投
	for (int i = 0; i < AIRDROP_NUM; i++)
	{
		if (airdrops[i].alive)
		{
			drawAlpha(&image_airdrop[airdrops[i].type], airdrops[i].x, airdrops[i].y);
		}
	}
	//绘制敌机
	for (int i = 0; i < ENEMY_NUM; i++)
	{
		// 只有非BOSS模式才绘制普通敌机
		if (!is_boss_mode && enemy[i].alive)
		{
			if (enemy[i].type == ENEMY_SMALL)
			{
				drawAlpha(&image_enemy[0], enemy[i].x, enemy[i].y);
			}
			//中型敌机
			if (enemy[i].type == ENEMY_MEDIUM)
			{
				drawAlpha(&image_enemy[1], enemy[i].x, enemy[i].y);

			}
			//大型敌机
			if (enemy[i].type == ENEMY_BIG)
			{
				drawAlpha(&image_enemy[2], enemy[i].x, enemy[i].y);
			}
		}
		//绘制敌机被击中动画
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
	
	// 绘制BOSS
	if (is_boss_mode)
	{
		if (boss1.alive)
		{
			drawAlpha(&image_boss1[0], boss1.x, boss1.y);
			
			// 绘制BOSS血条
			setfillcolor(RED);
			fillrectangle(boss1.x, boss1.y - 10, boss1.x + boss1.width * (boss1.hp / (float)boss1.max_hp), boss1.y - 5);
			setlinecolor(WHITE);
			rectangle(boss1.x, boss1.y - 10, boss1.x + boss1.width, boss1.y - 5);
		}
		else if (boss_defeated)
		{
			// 减慢动画帧切换速度，从150ms改为200ms
			if (timer_arrive(22, 200))
			{
				boss_explosion_frame++;
			}
			
			// 依次显示爆炸帧
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

	//设置重开弹窗
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

	//绘制子弹
	for (int i = 0; i < BULLET_NUM; i++)
	{
		if (bullet[i].alive)
		{
			// 根据子弹类型绘制，而非全局状态
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
	// 绘制护盾状态（左中位置显示）
    if (has_shield) {
        // 绘制护盾图标（使用已加载的image_airdrop[1]）
        drawAlpha(&image_airdrop[1], 20, 50);  // 坐标可根据实际调整
        
        // 绘制文字说明
        RECT shieldTextRect = { 60, 50, 200, 80 };  // 文字区域（图标右侧）
        settextstyle(20, 10, L"黑体");  // 设置字体大小
        settextcolor(WHITE);            // 白色文字
        drawtext(L"护盾激活", &shieldTextRect, DT_VCENTER | DT_LEFT);  // 垂直居中+左对齐
    }

    // 绘制无敌状态（左中位置显示）
    if (is_invincible) {
        // 计算剩余时间（秒）
        int remaining = max(0, (int)(invincible_end_time - clock()) / 1000);
        
        // 绘制护盾图标（使用已加载的image_airdrop[1]）
        drawAlpha(&image_airdrop[1], 20, 50);  // 坐标可根据实际调整
        
        // 绘制文字说明（显示剩余时间）
        RECT invincibleTextRect = { 60, 50, 200, 80 };  // 文字区域（图标右侧）
        settextstyle(20, 10, L"黑体");  // 设置字体大小
        settextcolor(WHITE);            // 白色文字
        wchar_t text[50];
        swprintf_s(text, L"无敌剩余：%d秒", remaining);
        drawtext(text, &invincibleTextRect, DT_VCENTER | DT_LEFT);  // 垂直居中+左对齐
    }
}
//生成敌机类型
void enemy_hp(int index)
{
	int num = rand();
	//大型敌机
	if (num%20 == 0)
	{
	enemy[index].type = ENEMY_BIG;
	enemy[index].hp = 3;
	enemy[index].height = 258;
	enemy[index].width = 169;
	}
	//中型敌机
	else if (num%20==2||num%20==4)
	{
		enemy[index].type = ENEMY_MEDIUM;
		enemy[index].hp = 2;
		enemy[index].height = 99;
		enemy[index].width = 69;
	}
	//小型敌机
	else
	{
		enemy[index].type = ENEMY_SMALL;
		enemy[index].hp = 1;
		enemy[index].height = 43;
		enemy[index].width = 57;
	}
}

//随机生成敌机的位置
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
			//重置死亡计数
			//fall_cnt[i] = 0;
			break;
		}
	}
}

//敌机移动
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

// BOSS移动
void boss_move(float speed)  // 修改参数为float类型以支持小数速度
{
	if (!boss1.alive) return;
	
	// 使用浮点数累积位移实现慢速移动
	static float move_offset = 0.0f;
	move_offset += speed;
	
	// 当累积位移达到1像素时才实际移动
	if (move_offset >= 1.0f || move_offset <= -1.0f)
	{
		boss1.x += boss1.move_direction * (int)move_offset;
		move_offset -= (int)move_offset;
	}
	
	// 边界检测，碰到边界改变方向
	if (boss1.x <= 0)
		boss1.move_direction = 1;
	if (boss1.x + boss1.width >= width)
		boss1.move_direction = -1;
}

// BOSS发射子弹
void boss_shoot()
{
	if (!boss1.alive) return;
	
	// 修复计时器ID冲突问题，使用唯一ID 20并增加调试输出
	if (timer_arrive(20, 1000)) // 1秒发射一次
	{
		for (int i = 0; i < BULLET_NUM; i++)
		{
			if (!bullet[i].alive)
			{
				bullet[i].x = boss1.x + boss1.width / 2;
				bullet[i].y = boss1.y + boss1.height;
				bullet[i].alive = true;
				bullet[i].type = 0; // 普通子弹
				break;
			}
		}
	}
}

// 检测BOSS与子弹碰撞（优化碰撞检测）
void check_boss_collision()
{
	if (!boss1.alive) return;
	
	for (int j = 0; j < BULLET_NUM; j++)
	{
		if (bullet[j].alive && bullet[j].type == 0) // 只允许普通子弹伤害BOSS
		{
			// 精确碰撞检测（覆盖子弹全部区域）
			if (bullet[j].x < boss1.x + boss1.width &&
				bullet[j].x + bullet[j].width > boss1.x &&
				bullet[j].y < boss1.y + boss1.height &&
				bullet[j].y + bullet[j].height > boss1.y)
			{
				boss1.hp--;
				bullet[j].alive = false;  // 立即销毁子弹
				
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

// 生成空投
void creat_airdrop() {
    for (int i = 0; i < AIRDROP_NUM; i++) {
        if (!airdrops[i].alive) {
            // 低概率生成空投(1/30概率)
            if (rand() % 30 == 0) {
                airdrops[i].x = rand() % (width - 50);
                airdrops[i].y = -50; // 从屏幕上方出现
                airdrops[i].width = 50;
                airdrops[i].height = 50;
                airdrops[i].alive = true;
                // 随机选择空投类型：0-黑洞子弹（50%），1-护盾（50%）
                airdrops[i].type = rand() % 2; 
                break;
            }
        }
    }
}

// 空投移动
void airdrop_move(int speed) {
    for (int i = 0; i < AIRDROP_NUM; i++) {
        if (airdrops[i].alive) {
            airdrops[i].y += speed;
            // 如果超出屏幕底部，设置为不活跃
            if (airdrops[i].y > height) {
                airdrops[i].alive = false;
            }
        }
    }
}

// 检测空投碰撞
// 检测空投碰撞
void check_airdrop_collision() {
    if (!player.alive) return;

    for (int i = 0; i < AIRDROP_NUM; i++) {
        if (airdrops[i].alive) {
            // 矩形碰撞检测
            if (player.x < airdrops[i].x + airdrops[i].width &&
                player.x + player.width > airdrops[i].x &&
                player.y < airdrops[i].y + airdrops[i].height &&
                player.y + player.height > airdrops[i].y) {
                // 根据类型触发不同效果
                if (airdrops[i].type == 0) {
                    // 黑洞子弹效果
                    is_blackhole_bullet = true;
                    blackhole_bullet_end_time = clock() + 5000;
                } else {
                    // 无敌效果（持续8秒）
                    is_invincible = true;
                    invincible_end_time = clock() + 8000;  // 8000ms=8秒
                }
                airdrops[i].alive = false; // 收集后消失
            }
        }
    }
}

void load_image()
{
	//加载本机图片
	loadimage(&image_player[0], _T("./images/me.png"), player.width, player.height);
	loadimage(&image_player[1], _T("./images/me_destroy_1.png"), player.width, player.height);
	loadimage(&image_player[2], _T("./images/me_destroy_2.png"), player.width, player.height);
	loadimage(&image_player[3], _T("./images/me_destroy_3.png"), player.width, player.height);
	loadimage(&image_player[4], _T("./images/me_destroy_4.png"), player.width, player.height);
	//加载背景图片
	loadimage(&image_background, _T("./images/background.png"), width, height);
	//加载敌机图片
	loadimage(&image_enemy[0], _T("./images/enemy1.png"));
	loadimage(&image_enemy[1], _T("./images/enemy2.png"));
	loadimage(&image_enemy[2], _T("./images/enemy3.png"));

	loadimage(&image_restart, _T("./images/again.png"));//加载重启按钮图片

	loadimage(&image_bullet, _T("./images/bullet.png"));//加载子弹
	// 添加最后一个参数true以32位色深加载，保留Alpha通道
	loadimage(&image_blackhole_bullet, _T("./images/blackhole_bullet.png"), 40, 40, true);//加载黑洞子弹

	loadimage(&image_enemy_fall_1[0], _T("./images/enemy1_down1.png"));//以下三种不同敌机被击中效果
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
	//加载空投图片，添加true参数保留Alpha通道
	loadimage(&image_airdrop[0], _T("./images/blackhole_airdrop.png"), 90, 90, true);
	loadimage(&image_airdrop[1], _T("./images/shields.png"), 60, 60, true);
	// 加载BOSS1图片
	loadimage(&image_boss1[0], _T("./images/boss1.png"), 150, 300, true);
	loadimage(&image_boss1[1], _T("./images/boss1_fall_1.png"), 150, 300, true);
	loadimage(&image_boss1[2], _T("./images/boss1_fall_2.png"), 150, 300, true);
	loadimage(&image_boss1[3], _T("./images/boss1_fall_3.png"), 200, 200, true);
}


void player_death()//本机死亡
{
    for (int i = 0; i < ENEMY_NUM; i++) {
        if (player.x + player.width < enemy[i].x || player.x > enemy[i].x + enemy[i].width || player.y + player.height < enemy[i].y || player.y > enemy[i].y + enemy[i].height) {
            continue;
        } else {
            if (is_invincible) {
                // 无敌期间不触发死亡，仅销毁敌机
                enemy[i].alive = false;
            } else {
                player.alive = false;
                player.fall = true;
                enemy[i].alive = false;
            }
        }
    }
}


// 为BOSS子弹添加单独的移动函数
void boss_bullet_move(int speed)
{
	for (int i = 0; i < BULLET_NUM; i++)
	{
		// 检测是否是BOSS发射的子弹（y方向向下移动）
		if (bullet[i].alive && bullet[i].y > player.y)
		{
			bullet[i].y += speed; // BOSS子弹向下移动
			if (bullet[i].y > height)
			{
				bullet[i].alive = false;
			}
		}
	}
}

// 修改BOSS战更新函数
void update_boss_battle(int speed)
{
	// 玩家操作
	if (timer_arrive(0, 10))
		player_move(speed);
	if (timer_arrive(1, 10))
		bullet_move(3); // 玩家子弹向上移动
		
	// BOSS行为
	if (boss1.alive)
	{
		boss_move(0.5f);  // 设置BOSS移动速度为0.5
		check_boss_collision();
	}
	// BOSS被击败后处理动画播放
	else if (boss_defeated)
	{
		// 每200ms切换一帧动画
		if (timer_arrive(22, 200)) {
			boss_explosion_frame++;
		}

		// 播放完所有动画帧后切换模式（保留boss_defeated为true）
		if (boss_explosion_frame >= 3) 
		{
			is_boss_mode = false;  // 切换回普通模式
			// 不立即重置boss_defeated，避免立即重新触发BOSS战
			// boss_defeated = false;
			boss_explosion_frame = 0;
			boss1.hp = boss1.max_hp;
			boss1.x = width / 2 - boss1.width / 2;
			boss1.y = 50;
			boss1.alive = false;
		}
	}
}

// 在普通游戏更新函数中添加boss_defeated重置逻辑
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

// 在main函数中使用新的分离函数
int main()
{
	srand((unsigned)time(NULL));//
	int speed = 5; //定义速度
	game_init(); //初始化游戏
	load_image();//加载所有图片
	BeginBatchDraw();
	while (1)
	{

		game_draw();
		FlushBatchDraw();
		// 只有当游戏未处于重启状态时，才执行游戏逻辑
		if (!restart)
		{
			// 检测分数是否达到100分，触发BOSS战 - 添加明确的触发分数常量
			const int BOSS_TRIGGER_SCORE = 100;
			if (score >= BOSS_TRIGGER_SCORE && !is_boss_mode && !boss_defeated)
			{
				is_boss_mode = true;
				boss1.alive = true;
				boss1.hp = boss1.max_hp; // 重置BOSS血量
				
				// 清空当前敌机和空投
				for (int i = 0; i < ENEMY_NUM; i++)
				{
					enemy[i].alive = false;
				}
				for (int i = 0; i < AIRDROP_NUM; i++)
				{
					airdrops[i].alive = false;
				}
			}
			
			// 根据游戏模式调用不同的更新函数
			if (is_boss_mode)
			{
				update_boss_battle(speed);
			}
			else
			{
				// 在普通模式下重置boss_defeated状态，允许下次达到分数时再次触发BOSS战
				// if (boss_defeated)
				// {
				// 	boss_defeated = false;
				// }
				update_normal_game(speed);
			}
			
			// 检查黑洞子弹是否过期
			if (is_blackhole_bullet && clock() > blackhole_bullet_end_time)
			{
				is_blackhole_bullet = false;
				// 清除所有活跃的黑洞子弹
				for (int i = 0; i < BULLET_NUM; i++)
				{
					if (bullet[i].alive && bullet[i].type == 1)
					{
						bullet[i].alive = false;
					}
				}
			}
			attack_plane(); // 子弹攻击敌机
			player_death(); // 检测玩家飞机是否死亡
		}
		else
		{
			// 仅保留重启功能
			if (GetAsyncKeyState(VK_RETURN))
			{
				restart = false;
				game_init(); // 重新初始化游戏状态
				is_blackhole_bullet = false; // 重置黑洞子弹状态
				is_boss_mode = false;
				boss_defeated = false;
			}
		}
	}
	EndBatchDraw();
	return 0;
}

// 图片透明化，会调用即可，用自带封装函数easyx不兼容无法透明化
void drawAlpha(IMAGE* picture, int picture_x, int picture_y) {
	DWORD* dst = GetImageBuffer(); // 获取目标绘图区缓冲区
	DWORD* src = GetImageBuffer(picture); // 获取源图片缓冲区（关键修正）

	int picture_width = picture->getwidth();
	int picture_height = picture->getheight();
	int graphWidth = getwidth();
	int graphHeight = getheight();

	for (int iy = 0; iy < picture_height; iy++) {
		for (int ix = 0; ix < picture_width; ix++) {
			// 计算源图像像素位置
			int srcX = ix + iy * picture_width;

			// 计算目标图像位置并检查边界
			int dstX = (ix + picture_x) + (iy + picture_y) * graphWidth;
			if (ix + picture_x < 0 || ix + picture_x >= graphWidth ||
				iy + picture_y < 0 || iy + picture_y >= graphHeight) {
				continue; // 跳过越界像素
			}

			// 提取alpha通道值
			int sa = ((src[srcX] & 0xff000000) >> 24);
			if (sa == 0) continue; // 完全透明像素跳过处理

			// 提取RGB分量
			int sr = ((src[srcX] & 0xff0000) >> 16);
			int sg = ((src[srcX] & 0xff00) >> 8);
			int sb = src[srcX] & 0xff;

			// 提取目标像素颜色
			int dr = ((dst[dstX] & 0xff0000) >> 16);
			int dg = ((dst[dstX] & 0xff00) >> 8);
			int db = dst[dstX] & 0xff;

			// 执行alpha混合
			dst[dstX] = (
				((sr * sa + dr * (255 - sa)) / 255) << 16 |
				((sg * sa + dg * (255 - sa)) / 255) << 8 |
				((sb * sa + db * (255 - sa)) / 255)
				);
		}
	}
}

bool timer_arrive(int id, int ms)//设置冷却时间，防止飞机移动过快，防止生成过快，敌机生成1，子弹冷却2，敌机移动3，本机死亡4
{
	static vector<DWORD> t(100);
	if (clock() - t[id] > ms)
	{
		t[id] = clock();
		return true;
	}
	return false;
}



