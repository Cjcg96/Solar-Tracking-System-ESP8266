% Configuración de ThingSpeak
channelID = 2869912; % Reemplaza con tu ID de canal
readAPIKey = 'TU_API_KEY'; % Reemplaza con tu API Key de lectura

% Nombre del archivo de Excel
filename = 'datos_proyecto.xlsx';

% Definir nombres de columnas
columnNames = {'Tiempo', 'Voltaje (V)', 'Corriente (A)', 'Potencia (W)', 'Inclinación (°)', 'Rotación (°)'};

% Crear la primera figura con gráficas
fig1 = figure('Name', 'Monitoreo en Tiempo Real', 'NumberTitle', 'off', 'WindowState', 'maximized');

% Crear subgráficas
subplot(2,1,1);
h1 = plot(nan, nan, 'r-o', 'LineWidth', 2, 'MarkerSize', 8); hold on;
h2 = plot(nan, nan, 'b-s', 'LineWidth', 2, 'MarkerSize', 8);
h3 = plot(nan, nan, 'g-d', 'LineWidth', 2, 'MarkerSize', 8);
grid on;
title('Monitoreo de Energía', 'FontSize', 16);
xlabel('Tiempo', 'FontSize', 14);
ylabel('Valores', 'FontSize', 14);
legend([h1, h2, h3], {'Voltaje (V)', 'Corriente (A)', 'Potencia (W)'}, 'Location', 'northeastoutside', 'FontSize', 12);

subplot(2,1,2);
h4 = plot(nan, nan, 'm-*', 'LineWidth', 2, 'MarkerSize', 8); hold on;
h5 = plot(nan, nan, 'c-^', 'LineWidth', 2, 'MarkerSize', 8);
grid on;
title('Movimiento del Panel Solar', 'FontSize', 16);
xlabel('Tiempo', 'FontSize', 14);
ylabel('Ángulo (°)', 'FontSize', 14);
legend([h4, h5], {'Inclinación (°)', 'Rotación (°)'}, 'Location', 'northeastoutside', 'FontSize', 12);

% Crear la segunda figura para la tabla con tamaño ajustado
fig2 = figure('Name', 'Datos en Tiempo Real', 'NumberTitle', 'off', 'Position', [100, 100, 600, 250]);
t = uitable(fig2, 'Data', cell(0, 6), 'ColumnName', columnNames, ...
            'Position', [20, 20, 560, 200], 'FontSize', 12);

% Variables para almacenar datos
timeData = datetime.empty;
voltageData = [];
currentData = [];
powerData = [];
servoVData = [];
servoHData = [];

while ishandle(fig1) && ishandle(fig2)
    % Leer datos de ThingSpeak
    data = thingSpeakRead(channelID, 'ReadKey', readAPIKey, 'NumPoints', 1);
    
    % Extraer valores
    voltage = data(1);
    current = data(2);
    power = data(3);
    servov = data(4); % Inclinación del panel
    servoh = data(5); % Rotación de la base
    
    % Guardar datos con el tiempo actual
    timeNow = datetime('now', 'Format', 'yyyy-MM-dd HH:mm:ss');
    timeData = [timeData; timeNow];
    voltageData = [voltageData; voltage];
    currentData = [currentData; current];
    powerData = [powerData; power];
    servoVData = [servoVData; servov];
    servoHData = [servoHData; servoh];

    % Limitar el número de puntos a los últimos 50 en la gráfica
    if length(timeData) > 50
        timeData(1) = [];
        voltageData(1) = [];
        currentData(1) = [];
        powerData(1) = [];
        servoVData(1) = [];
        servoHData(1) = [];
    end

    % Actualizar gráficas
    set(h1, 'XData', datenum(timeData), 'YData', voltageData);
    set(h2, 'XData', datenum(timeData), 'YData', currentData);
    set(h3, 'XData', datenum(timeData), 'YData', powerData);
    set(h4, 'XData', datenum(timeData), 'YData', servoVData);
    set(h5, 'XData', datenum(timeData), 'YData', servoHData);
    datetick('x', 'HH:MM:SS', 'keeplimits'); % Formato de hora en el eje X

    % Actualizar la tabla con los últimos datos
    newData = {char(timeNow), voltage, current, power, servov, servoh};
    currentTableData = t.Data;
    
    % Mantener solo 10 filas en la tabla visual
    if size(currentTableData, 1) >= 10
        currentTableData(1, :) = []; % Eliminar la primera fila si hay más de 10 registros
    end
    
    t.Data = [currentTableData; newData]; % Agregar nueva fila a la tabla visual

    % 📌 **Actualizar archivo Excel correctamente**
    % Verificar si el archivo ya existe
    if exist(filename, 'file')
        % Leer datos actuales con nombres de columnas originales
        oldData = readtable(filename, 'VariableNamingRule', 'preserve');
        
        % Crear una nueva tabla con datos recientes
        newRow = cell2table(newData, 'VariableNames', oldData.Properties.VariableNames);
        
        % Concatenar datos sin cambiar el formato
        updatedTable = [oldData; newRow];
        
        % Escribir la tabla actualizada
        writetable(updatedTable, filename);
    else
        % Si el archivo no existe, crear una nueva tabla con las columnas correctas
        newTable = cell2table(newData, 'VariableNames', columnNames);
        writetable(newTable, filename);
    end

    % Refrescar gráficos y tabla
    drawnow;
    pause(60);
end

