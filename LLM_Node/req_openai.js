const dotenv = require("dotenv");
dotenv.config();

const OpenAI = require("openai");

const openai = new OpenAI({
    apiKey: process.env.OPENAI_API_KEY,
});

const callLLM = async (playerStateJson) => {
    try {
        console.log("플레이어 상태 수신:", playerStateJson);

        const response = await openai.chat.completions.create({
            model: "gpt-4o",
            // JSON 모드 활성화 (파싱 에러 방지)
            response_format: { type: "json_object" }, 
            messages: [
                { 
                    role: "system", 
                    content: `
                    너는 '적응형 게임 AI 디렉터'다. 
                    너의 목표는 플레이어의 실력과 현재 상태(State)를 분석하여, 
                    가장 몰입감(Flow)을 느낄 수 있는 다음 맵의 PCG 파라미터(Action)를 결정하는 것이다.

                    다음 규칙을 따라 JSON 형식으로만 응답해라:
                    1. 플레이어가 체력이 높고 적을 빨리 죽임 -> '난이도 상승', '함정 추가', '적 공격성 증가'
                    2. 플레이어가 자주 죽거나 진행이 느림 -> '난이도 하락', '엄폐물 추가', '회복 아이템 증가'
                    3. 설명 없이 오직 JSON 데이터만 반환해라.
                    
                    반환해야 할 JSON 포맷:
                    {
                        "analysis": "플레이어가 지루해함(Easy) -> 긴장감 조성 필요",
                        "difficultyMultiplier": 1.0 ~ 5.0 (float),
                        "enemySpawnRate": 0.0 ~ 1.0 (float, 적 밀도),
                        "enemyAggression": "Low" | "Medium" | "High" (적 AI 공격성),
                        "mapComplexity": 0.0 ~ 1.0 (float, 미로 복잡도),
                        "obstacleType": "Cover" | "Trap" | "OpenArea",
                        "atmosphere": "Dark_Foggy" | "Bright_Clear" | "Red_Alarm"
                    }
                    ` 
                },
                { 
                    role: "user", 
                    // 언리얼에서 보낸 플레이어 데이터(JSON 문자열)가 여기에 들어감
                    content: `현재 플레이어 상태 데이터: ${playerStateJson}` 
                }
            ],
        });

        return response.choices[0].message.content;

    } catch (error) {
        console.error("OpenAI 호출 중 에러:", error);
        // 에러 발생 시 기본값 반환 (게임이 멈추지 않게)
        return JSON.stringify({
            analysis: "에러 발생 - 기본값 적용",
            difficultyMultiplier: 1.0,
            enemySpawnRate: 0.5,
            enemyAggression: "Medium",
            mapComplexity: 0.5,
            obstacleType: "Cover",
            atmosphere: "Bright_Clear"
        });
    }
};

module.exports = { callLLM };