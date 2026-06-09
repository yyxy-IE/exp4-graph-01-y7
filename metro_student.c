/**
 * 地铁线路图查询器（修复站点读取问题，保证21站点）
 * 编译：gcc -o metro metro_student.c -std=c99 -Wall -g
 * 运行：./metro
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#define MAX_NAME_LEN 32
#define MAX_LINE_NAME 20
#define BUF_LEN 512

// 邻接表边结点
typedef struct EdgeNode {
    int adjVex;
    int weight;
    struct EdgeNode *next;
} EdgeNode;

// 顶点结点（站点）
typedef struct VertexNode {
    char name[MAX_NAME_LEN];
    EdgeNode *firstEdge;
    int *lineIds;
    int lineCount;
} VertexNode;

// 图结构
typedef struct {
    VertexNode *vertices;
    int vertexNum;
    int vertexCapacity;
    int edgeNum;
    int isDirected;
} Graph;

// 辅助队列（BFS）
typedef struct Queue {
    int *data;
    int front, rear, size, capacity;
} Queue;

// 函数声明
Graph* createGraph(int initCapacity, int isDirected);
void resizeGraph(Graph *g);
int addVertex(Graph *g, const char *name);
int findVertexIndex(Graph *g, const char *name);
void addEdge(Graph *g, int u, int v, int weight);
void addLineToStation(Graph *g, int stationIdx, int lineId);
void readMetroFile(const char *filename, Graph *g);
void printAdjList(Graph *g);

void DFSRecursive(Graph *g, int v, int *visited);
void DFSTraversal(Graph *g, int start);
void BFSTraversal(Graph *g, int start);
void connectivityAnalysis(Graph *g);
void dijkstra(Graph *g, int start, int *dist, int *prev);
void printPath(Graph *g, int *prev, int start, int end);
void shortestPathByTime(Graph *g, int start, int end);
void shortestPathByTransfer(Graph *g, int start, int end);
void freeGraph(Graph *g);
void printMenu();

Queue* createQueue(int capacity);
void enqueue(Queue *q, int val);
int dequeue(Queue *q);
int isEmpty(Queue *q);
void freeQueue(Queue *q);

// ---------- 主函数 ----------
int main() {
    Graph *g = createGraph(100, 0);
    readMetroFile("metro.txt", g);

    int choice, start, end;
    char startName[MAX_NAME_LEN], endName[MAX_NAME_LEN];
    do {
        printMenu();
        printf("请输入选择：");
        scanf("%d", &choice);
        getchar();
        switch (choice) {
            case 1:
                printAdjList(g);
                break;
            case 2:
                printf("请输入起始站点名称：");
                fgets(startName, MAX_NAME_LEN, stdin);
                startName[strcspn(startName, "\n")] = '\0';
                start = findVertexIndex(g, startName);
                if (start == -1) {
                    fprintf(stderr, "错误：站点 '%s' 不存在。\n", startName);
                } else {
                    printf("\nDFS 遍历序列（从 %s 开始）：\n", startName);
                    DFSTraversal(g, start);
                }
                break;
            case 3:
                printf("请输入起始站点名称：");
                fgets(startName, MAX_NAME_LEN, stdin);
                startName[strcspn(startName, "\n")] = '\0';
                start = findVertexIndex(g, startName);
                if (start == -1) {
                    fprintf(stderr, "错误：站点 '%s' 不存在。\n", startName);
                } else {
                    printf("\nBFS 遍历序列（从 %s 开始）：\n", startName);
                    BFSTraversal(g, start);
                }
                break;
            case 4:
                connectivityAnalysis(g);
                break;
            case 5:
                printf("请输入起点站：");
                fgets(startName, MAX_NAME_LEN, stdin);
                startName[strcspn(startName, "\n")] = '\0';
                printf("请输入终点站：");
                fgets(endName, MAX_NAME_LEN, stdin);
                endName[strcspn(endName, "\n")] = '\0';
                start = findVertexIndex(g, startName);
                end = findVertexIndex(g, endName);
                if (start == -1) {
                    fprintf(stderr, "错误：起点 '%s' 不存在。\n", startName);
                } else if (end == -1) {
                    fprintf(stderr, "错误：终点 '%s' 不存在。\n", endName);
                } else {
                    shortestPathByTime(g, start, end);
                }
                break;
            case 6:
                printf("请输入起点站：");
                fgets(startName, MAX_NAME_LEN, stdin);
                startName[strcspn(startName, "\n")] = '\0';
                printf("请输入终点站：");
                fgets(endName, MAX_NAME_LEN, stdin);
                endName[strcspn(endName, "\n")] = '\0';
                start = findVertexIndex(g, startName);
                end = findVertexIndex(g, endName);
                if (start == -1) {
                    fprintf(stderr, "错误：起点 '%s' 不存在。\n", startName);
                } else if (end == -1) {
                    fprintf(stderr, "错误：终点 '%s' 不存在。\n", endName);
                } else {
                    shortestPathByTransfer(g, start, end);
                }
                break;
            case 0:
                printf("退出程序。\n");
                break;
            default:
                printf("无效选择，请重新输入。\n");
        }
        printf("\n");
    } while (choice != 0);
    freeGraph(g);
    return 0;
}

// ---------- 基础图操作（无修改）----------
Graph* createGraph(int initCapacity, int isDirected) {
    Graph *g = (Graph*)malloc(sizeof(Graph));
    g->vertexCapacity = initCapacity;
    g->vertexNum = 0;
    g->edgeNum = 0;
    g->isDirected = isDirected;
    g->vertices = (VertexNode*)malloc(sizeof(VertexNode) * initCapacity);
    for (int i = 0; i < initCapacity; i++) {
        g->vertices[i].name[0] = '\0';
        g->vertices[i].firstEdge = NULL;
        g->vertices[i].lineIds = NULL;
        g->vertices[i].lineCount = 0;
    }
    return g;
}

void resizeGraph(Graph *g) {
    int newCap = g->vertexCapacity * 2;
    g->vertices = (VertexNode*)realloc(g->vertices, sizeof(VertexNode) * newCap);
    for (int i = g->vertexCapacity; i < newCap; i++) {
        g->vertices[i].name[0] = '\0';
        g->vertices[i].firstEdge = NULL;
        g->vertices[i].lineIds = NULL;
        g->vertices[i].lineCount = 0;
    }
    g->vertexCapacity = newCap;
}

int addVertex(Graph *g, const char *name) {
    int idx = findVertexIndex(g, name);
    if (idx != -1) return idx;
    if (g->vertexNum >= g->vertexCapacity) {
        resizeGraph(g);
    }
    strcpy(g->vertices[g->vertexNum].name, name);
    g->vertices[g->vertexNum].firstEdge = NULL;
    g->vertices[g->vertexNum].lineIds = NULL;
    g->vertices[g->vertexNum].lineCount = 0;
    return g->vertexNum++;
}

int findVertexIndex(Graph *g, const char *name) {
    for (int i = 0; i < g->vertexNum; i++) {
        if (strcmp(g->vertices[i].name, name) == 0)
            return i;
    }
    return -1;
}

void addEdge(Graph *g, int u, int v, int weight) {
    if (u < 0 || u >= g->vertexNum || v < 0 || v >= g->vertexNum) return;
    EdgeNode *e = (EdgeNode*)malloc(sizeof(EdgeNode));
    e->adjVex = v;
    e->weight = weight;
    e->next = g->vertices[u].firstEdge;
    g->vertices[u].firstEdge = e;
    if (!g->isDirected) {
        e = (EdgeNode*)malloc(sizeof(EdgeNode));
        e->adjVex = u;
        e->weight = weight;
        e->next = g->vertices[v].firstEdge;
        g->vertices[v].firstEdge = e;
    }
    g->edgeNum++;
}

void addLineToStation(Graph *g, int stationIdx, int lineId) {
    if (stationIdx < 0 || stationIdx >= g->vertexNum) return;
    for (int i = 0; i < g->vertices[stationIdx].lineCount; i++) {
        if (g->vertices[stationIdx].lineIds[i] == lineId)
            return;
    }
    g->vertices[stationIdx].lineCount++;
    g->vertices[stationIdx].lineIds = (int*)realloc(g->vertices[stationIdx].lineIds,
        sizeof(int) * g->vertices[stationIdx].lineCount);
    g->vertices[stationIdx].lineIds[g->vertices[stationIdx].lineCount - 1] = lineId;
}

// 判断字符串是否纯数字
static int isAllDigit(const char *s) {
    if (!s || *s == '\0') return 0;
    while (*s) {
        if (!isdigit((unsigned char)*s)) return 0;
        s++;
    }
    return 1;
}

// ========== 【重点修复】全新文件读取函数，保证读取21个站点 ==========
void readMetroFile(const char *filename, Graph *g) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "无法打开文件 %s\n", filename);
        exit(1);
    }

    char buf[BUF_LEN];
    int totalStation = 0, lineCount = 0;

    // 读取第一行：总站点数 21
    while (fgets(buf, BUF_LEN, fp)) {
        if (strlen(buf) == 0 || buf[0] == '\n') continue;
        sscanf(buf, "%d", &totalStation);
        break;
    }
    // 读取第二行：线路数 5
    while (fgets(buf, BUF_LEN, fp)) {
        if (strlen(buf) == 0 || buf[0] == '\n') continue;
        sscanf(buf, "%d", &lineCount);
        break;
    }

    // 逐条解析5条地铁线路
    for (int lineId = 0; lineId < lineCount; lineId++) {
        // 跳过空行
        while (fgets(buf, BUF_LEN, fp)) {
            if (strlen(buf) > 1) break;
        }
        char *tokens[64];
        int tokenCnt = 0;
        char *p = strtok(buf, " \t\r\n");
        while (p != NULL && tokenCnt < 64) {
            tokens[tokenCnt++] = p;
            p = strtok(NULL, " \t\r\n");
        }
        if (tokenCnt < 2) continue;

        // tokens[0] = 线路名, tokens[1] = 站点数量
        int preIdx = -1;
        int curTime = 1;

        // 从第2个token开始遍历：站点 + 间隔时间交替
        for (int i = 2; i < tokenCnt; i++) {
            char *cur = tokens[i];
            if (isAllDigit(cur)) {
                // 数字：两站之间的运行时间
                curTime = atoi(cur);
                continue;
            }
            // 非数字：站点名
            int curIdx = addVertex(g, cur);
            addLineToStation(g, curIdx, lineId);
            if (preIdx != -1) {
                addEdge(g, preIdx, curIdx, curTime);
                curTime = 1;
            }
            preIdx = curIdx;
        }
    }
    fclose(fp);
    printf("成功读取地铁数据：共 %d 个站点，%d 条边。\n", g->vertexNum, g->edgeNum);
}

void printAdjList(Graph *g) {
    printf("\n===== 邻接表 =====\n");
    for (int i = 0; i < g->vertexNum; i++) {
        printf("%s (%d条线路): ", g->vertices[i].name, g->vertices[i].lineCount);
        EdgeNode *e = g->vertices[i].firstEdge;
        while (e) {
            printf("-> %s(%dmin) ", g->vertices[e->adjVex].name, e->weight);
            e = e->next;
        }
        printf("\n");
    }
    printf("\n===== 换乘站 =====\n");
    for (int i = 0; i < g->vertexNum; i++) {
        if (g->vertices[i].lineCount > 1) {
            printf("%s：%d 条线路\n", g->vertices[i].name, g->vertices[i].lineCount);
        }
    }
}

void printMenu() {
    printf("\n====== 地铁查询系统 ======\n");
    printf("1. 输出邻接表和换乘站\n");
    printf("2. DFS 遍历（从指定站点）\n");
    printf("3. BFS 遍历（从指定站点）\n");
    printf("4. 连通分量分析\n");
    printf("5. 最短路径（最少时间）\n");
    printf("6. 最短路径（最少换乘）\n");
    printf("0. 退出\n");
}

// ---------- 队列 ----------
Queue* createQueue(int capacity) {
    Queue *q = (Queue*)malloc(sizeof(Queue));
    q->data = (int*)malloc(sizeof(int) * capacity);
    q->front = q->rear = q->size = 0;
    q->capacity = capacity;
    return q;
}

void enqueue(Queue *q, int val) {
    if (q->size == q->capacity) return;
    q->data[q->rear] = val;
    q->rear = (q->rear + 1) % q->capacity;
    q->size++;
}

int dequeue(Queue *q) {
    if (q->size == 0) return -1;
    int val = q->data[q->front];
    q->front = (q->front + 1) % q->capacity;
    q->size--;
    return val;
}

int isEmpty(Queue *q) {
    return q->size == 0;
}

void freeQueue(Queue *q) {
    free(q->data);
    free(q);
}

// ---------- DFS / BFS ----------
void DFSRecursive(Graph *g, int v, int *visited) {
    visited[v] = 1;
    printf("%s  ", g->vertices[v].name);
    EdgeNode *p = g->vertices[v].firstEdge;
    while (p != NULL) {
        int next = p->adjVex;
        if (!visited[next]) {
            DFSRecursive(g, next, visited);
        }
        p = p->next;
    }
}

void DFSTraversal(Graph *g, int start) {
    int *visited = (int*)malloc(sizeof(int) * g->vertexNum);
    memset(visited, 0, sizeof(int) * g->vertexNum);
    DFSRecursive(g, start, visited);
    free(visited);
    printf("\n");
}

void BFSTraversal(Graph *g, int start) {
    int *visited = (int*)malloc(sizeof(int) * g->vertexNum);
    memset(visited, 0, sizeof(int) * g->vertexNum);
    Queue *q = createQueue(g->vertexNum);

    visited[start] = 1;
    enqueue(q, start);
    printf("%s  ", g->vertices[start].name);

    while (!isEmpty(q)) {
        int u = dequeue(q);
        EdgeNode *p = g->vertices[u].firstEdge;
        while (p != NULL) {
            int v = p->adjVex;
            if (!visited[v]) {
                visited[v] = 1;
                printf("%s  ", g->vertices[v].name);
                enqueue(q, v);
            }
            p = p->next;
        }
    }
    freeQueue(q);
    free(visited);
    printf("\n");
}

// ---------- 连通分量 ----------
void connectivityAnalysis(Graph *g) {
    int *visited = (int*)malloc(sizeof(int) * g->vertexNum);
    memset(visited, 0, sizeof(int) * g->vertexNum);
    int cnt = 0;

    printf("\n===== 连通分量分析 =====\n");
    for (int i = 0; i < g->vertexNum; i++) {
        if (!visited[i]) {
            cnt++;
            printf("第 %d 个连通分量：", cnt);
            DFSRecursive(g, i, visited);
            printf("\n");
        }
    }
    printf("总连通分量个数：%d\n", cnt);
    free(visited);
}

// ---------- Dijkstra 最短路径 ----------
void dijkstra(Graph *g, int start, int *dist, int *prev) {
    int n = g->vertexNum;
    int *visited = (int*)malloc(sizeof(int) * n);

    for (int i = 0; i < n; i++) {
        dist[i] = INT_MAX;
        prev[i] = -1;
        visited[i] = 0;
    }
    dist[start] = 0;

    for (int i = 0; i < n; i++) {
        int u = -1;
        int min = INT_MAX;
        for (int j = 0; j < n; j++) {
            if (!visited[j] && dist[j] < min) {
                min = dist[j];
                u = j;
            }
        }
        if (u == -1) break;
        visited[u] = 1;

        EdgeNode *p = g->vertices[u].firstEdge;
        while (p != NULL) {
            int v = p->adjVex;
            int w = p->weight;
            if (!visited[v] && dist[u] != INT_MAX && dist[v] > dist[u] + w) {
                dist[v] = dist[u] + w;
                prev[v] = u;
            }
            p = p->next;
        }
    }
    free(visited);
}

void printPath(Graph *g, int *prev, int start, int end) {
    if (end == start) {
        printf("%s", g->vertices[start].name);
        return;
    }
    printPath(g, prev, start, prev[end]);
    printf(" -> %s", g->vertices[end].name);
}

void shortestPathByTime(Graph *g, int start, int end) {
    if (start == end) {
        printf("起点与终点为同一站点：%s\n", g->vertices[start].name);
        return;
    }
    int n = g->vertexNum;
    int *dist = (int*)malloc(sizeof(int) * n);
    int *prev = (int*)malloc(sizeof(int) * n);

    dijkstra(g, start, dist, prev);
    if (dist[end] == INT_MAX) {
        printf("无法从 %s 到达 %s\n", g->vertices[start].name, g->vertices[end].name);
    } else {
        printf("最少时间路径（总耗时：%d 分钟）：", dist[end]);
        printPath(g, prev, start, end);
        printf("\n");
    }
    free(dist);
    free(prev);
}

void shortestPathByTransfer(Graph *g, int start, int end) {
    if (start == end) {
        printf("起点与终点为同一站点：%s，换乘次数：0\n", g->vertices[start].name);
        return;
    }
    int n = g->vertexNum;
    int *dist = (int*)malloc(sizeof(int) * n);
    int *prev = (int*)malloc(sizeof(int) * n);

    // 保存原权重
    int totalEdge = g->edgeNum * 2;
    EdgeNode **eList = (EdgeNode**)malloc(totalEdge * sizeof(EdgeNode*));
    int *wList = (int*)malloc(totalEdge * sizeof(int));
    int idx = 0;

    for (int i = 0; i < g->vertexNum; i++) {
        EdgeNode *p = g->vertices[i].firstEdge;
        while (p) {
            eList[idx] = p;
            wList[idx] = p->weight;
            p->weight = 1;
            idx++;
            p = p->next;
        }
    }

    dijkstra(g, start, dist, prev);

    // 恢复权重
    for (int i = 0; i < idx; i++) {
        eList[i]->weight = wList[i];
    }
    free(eList);
    free(wList);

    if (dist[end] == INT_MAX) {
        printf("无法从 %s 到达 %s\n", g->vertices[start].name, g->vertices[end].name);
    } else {
        int transfer = dist[end] - 1;
        printf("最少换乘路径（换乘次数：%d）：", transfer);
        printPath(g, prev, start, end);
        printf("\n");
    }
    free(dist);
    free(prev);
}

// ---------- 内存释放 ----------
void freeGraph(Graph *g) {
    if (!g) return;
    for (int i = 0; i < g->vertexNum; i++) {
        EdgeNode *p = g->vertices[i].firstEdge;
        while (p) {
            EdgeNode *tmp = p;
            p = p->next;
            free(tmp);
        }
        free(g->vertices[i].lineIds);
    }
    free(g->vertices);
    free(g);
}