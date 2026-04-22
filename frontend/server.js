const express = require('express');
const { createProxyMiddleware } = require('http-proxy-middleware');
const app = express();
const port = 1488;
app.listen(port, () => {
    console.log(`Server is running on http://localhost:${port}`);
});
app.use(express.static('www'));

// Проксирование всех запросов с /proxy на http://atuin.space:8888
app.use('/proxy', createProxyMiddleware({
    target: 'http://atuin.space:8888',
    changeOrigin: true,
    pathRewrite: {
        '^/proxy': '',
    },
}));
app.use((req, res, next) => {
    res.setHeader('Access-Control-Allow-Headers', 'Content-Type, Authorization, X-Requested-With');
    res.setHeader('Access-Control-Allow-Methods', 'OPTIONS, GET, POST');
    res.setHeader('Access-Control-Allow-Origin', '*');
    next();
});
app.get('/', (req, res) => {
    res.sendFile(__dirname + '/www/index.html');
})