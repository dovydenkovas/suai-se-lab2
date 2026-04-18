const express = require('express');
const app = express();
const port = 1488;
app.listen(port, () => {
    console.log(`Server is running on http://localhost:${port}`);
});
app.use(express.static('www'));
app.get('/', (req, res) => {
    res.sendFile(__dirname + '/www/index.html');
})