<?php
// Mokthyn 更新检查接口：只输出纯文本版本号，供客户端启动时比对
// 发布新版本时，把下面这一行的数字改成新高版本号即可（例如 0.1.4）
header('Content-Type: text/plain; charset=utf-8');
header('Cache-Control: no-store, no-cache, must-revalidate, max-age=0');

echo '0.1.3';
