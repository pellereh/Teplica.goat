#ifndef PAGE_H
#define PAGE_H

const char PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="ru">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Умная теплица</title>
    <style>
        /* Общие стили */
        body {
            font-family: Arial, sans-serif;
            background-color: #faf3e0; /* Цвет фона - слоновая кость */
            color: #333333; /* Темно-серый текст */
            margin: 0;
            padding: 20px;
        }

        h1 {
            text-align: center;
            color: #555555; /* Темно-серый заголовок */
        }

        /* Контейнер для блоков */
        .container {
            display: grid; /* Используем CSS Grid */
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); /* Адаптивная сетка */
            gap: 20px; /* Расстояние между блоками */
            max-width: 1200px; /* Максимальная ширина контейнера */
            margin: 0 auto; /* Центрирование контейнера */
        }

        /* Стиль для каждого блока */
        .block-thin-tab {
            background-color: #f4e7d3; /* Фон блока - чуть темнее слоновой кости */
            border-radius: 12px; /* Закругленные углы */
            box-shadow: 0 6px 12px rgba(0, 0, 0, 0.1); /* Тень для блока */
            padding: 20px; /* Внутренний отступ */
            box-sizing: border-box; /* Корректное вычисление размеров */
        }

        /* Переключатели (switches) */
        .switch {
            position: relative;
            display: inline-block;
            width: 60px;
            height: 34px;
        }

        .switch input {
            opacity: 0;
            width: 0;
            height: 0;
        }

        .slider {
            position: absolute;
            cursor: pointer;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background-color: #ccc; /* Серый фон переключателя */
            transition: 0.3s;
            border-radius: 34px; /* Закругленные углы */
        }

        .slider:before {
            position: absolute;
            content: "";
            height: 26px;
            width: 26px;
            left: 4px;
            bottom: 4px;
            background-color: white; /* Белый круг внутри переключателя */
            transition: 0.3s;
            border-radius: 50%; /* Круглая форма */
        }

        input:checked + .slider {
            background-color: #27ae60; /* Зеленый цвет при включении */
        }

        input:checked + .slider:before {
            transform: translateX(26px); /* Сдвиг круга вправо */
        }

        /* Ползунок яркости */
        .light-slider {
            width: 100%;
            margin-top: 10px;
        }

        .light-value {
            text-align: center;
            margin-top: 5px;
            font-size: 14px;
            color: #555555; /* Темно-серый текст */
        }

        /* Кнопки управления */
        .button-container {
            display: flex;
            justify-content: space-around;
            margin-top: 10px;
        }

        .control-button {
            padding: 10px 20px;
            background-color: #27ae60; /* Зеленый фон кнопки */
            color: white;
            border: none;
            border-radius: 5px;
            cursor: pointer;
            transition: background-color 0.3s;
        }

        .control-button:hover {
            background-color: #218c4e; /* Темно-зеленый при наведении */
        }

        /* Данные датчиков */
        .sensor-data {
            font-size: 16px;
            margin-top: 10px;
            line-height: 1.5; /* Улучшение читаемости */
        }
    </style>
</head>
<body>
    <h1>Умная теплица</h1>

    <div class="container">
        <!-- Форточка -->
        <div class="block-thin-tab">
            <div class="switch-container">
                <label for="window-switch">Форточка:</label>
                <label class="switch">
                    <input type="checkbox" id="window-switch" %WINDOW_STATE%>
                    <span class="slider round"></span>
                </label>
            </div>
        </div>

        <!-- Насос -->
        <div class="block-thin-tab">
            <div class="switch-container">
                <label for="pump-switch">Насос:</label>
                <label class="switch">
                    <input type="checkbox" id="pump-switch" %PUMP_STATE%>
                    <span class="slider round"></span>
                </label>
            </div>
        </div>

        <!-- Вентилятор -->
        <div class="block-thin-tab">
            <div class="switch-container">
                <label for="wind-switch">Вентилятор:</label>
                <label class="switch">
                    <input type="checkbox" id="wind-switch" %WIND_STATE%>
                    <span class="slider round"></span>
                </label>
            </div>
        </div>

        <!-- Освещение -->
        <div class="block-thin-tab">
            <div class="switch-container">
                <label for="light-switch">Освещение:</label>
                <label class="switch">
                    <input type="checkbox" id="light-switch" %LIGHT_STATE%>
                    <span class="slider round"></span>
                </label>
            </div>
            <div class="light-container">
                <label for="light-slider">Яркость:</label>
                <input type="range" id="light-slider" min="0" max="100" value="%BRIGHTNESS%" class="light-slider">
                <div class="light-value" id="light-value">%BRIGHTNESS%%</div>
            </div>
        </div>

        <!-- Датчик температуры и влажности (BME280) -->
        <div class="block-thin-tab">
            <h3>Температура и влажность</h3>
            <div class="sensor-data">
                Температура: <span id="temperature">%TEMPERATURE% °C</span><br>
                Влажность: <span id="humidity">%HUMIDITY% %</span><br>
                Давление: <span id="pressure">%PRESSURE% hPa</span>
            </div>
        </div>

        <!-- Датчик освещенности (BH1750) -->
        <div class="block-thin-tab">
            <h3>Освещенность</h3>
            <div class="sensor-data">
                Уровень света: <span id="lux">%LUX% лк</span>
            </div>
        </div>
    </div>

    <script>
        // Функция для обновления данных с датчиков
        function updateSensorData() {
            fetch('/sensor/data')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('temperature').textContent = data.temperature + ' °C';
                    document.getElementById('humidity').textContent = data.humidity + ' %';
                    document.getElementById('pressure').textContent = data.pressure + ' hPa';
                    document.getElementById('lux').textContent = data.lux + ' лк';
                })
                .catch(error => console.error('Ошибка при получении данных:', error));
        }

        // Обновление данных каждые 2 секунды
        setInterval(updateSensorData, 2000);

        // Управление форточкой
        const windowSwitch = document.getElementById('window-switch');
        windowSwitch.addEventListener('change', function() {
            fetch(`/window/${this.checked ? 'open' : 'close'}`);
        });

        // Управление насосом
        const pumpSwitch = document.getElementById('pump-switch');
        pumpSwitch.addEventListener('change', function() {
            fetch(`/pump/${this.checked ? 'on' : 'off'}`);
        });

        // Управление вентилятором
        const windSwitch = document.getElementById('wind-switch');
        windSwitch.addEventListener('change', function() {
            fetch(`/wind/${this.checked ? 'on' : 'off'}`);
        });

        // Управление освещением через слайдер
        const lightSlider = document.getElementById('light-slider');
        const lightValue = document.getElementById('light-value');

        lightSlider.addEventListener('input', function() {
            lightValue.textContent = `${this.value}%`;
            fetch(`/light/brightness/?value=${this.value}`);
        });

        // Управление освещением через переключатель
        const lightSwitch = document.getElementById('light-switch');
        lightSwitch.addEventListener('change', function() {
            fetch(`/light/${this.checked ? 'on' : 'off'}`);
        });
    </script>
</body>
</html>
)rawliteral";

#endif // PAGE_H