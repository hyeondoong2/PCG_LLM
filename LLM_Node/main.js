const express = require('express');
const {callLLM} = require("./req_openai");

const app = express();
const port = 3000;

// JSON 파싱 미들웨어
app.use(express.json());

app.post('/api/openai', async (req, res) => {
    try {
        console.log("언리얼에서 받은 요청:", req.body);

        const prompt = req.body.data;         // UE에서 data 키로 보냄
        const llmAnswer = await callLLM(prompt); // GPT 호출

        console.log(llmAnswer);

        const result = {
            status: "ok",
            answer: llmAnswer,       // ← 언리얼로 보낼 GPT 응답
            receivedData: req.body,  // ← 디버그용
        };

        return res.json(result);

    } catch (error) {
        console.error("오류 발생:", error);
        return res.status(500).json({ error: "서버 내부 오류" });
    }
});

// 서버 실행
app.listen(port, () => {
    console.log(`REST API 서버가 http://localhost:${port} 에서 실행 중`);
});