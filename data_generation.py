import csv
import time
import random
import threading
from datetime import datetime
import os

class AdvancedOrderGenerator:
    def __init__(self, filename='orders.csv', show_live_preview=True):
        self.filename = filename
        self.show_live_preview = show_live_preview
        self.running = True
        self.row_count = 0
        self.start_time = time.time()
        
        # Параметры рынка
        self.base_price = 68480.0
        self.current_price = self.base_price
        self.price_trend = 0.0
        self.volatility = 0.1
        
        # Создаем или очищаем файл
        with open(self.filename, 'w', newline='', encoding='utf-8') as csvfile:
            fieldnames = ['receive_ts', 'exchange_ts', 'price', 'quantity', 'side']
            writer = csv.DictWriter(csvfile, fieldnames=fieldnames, delimiter=';')
            writer.writeheader()
        
        print(f"Файл инициализирован: {self.filename}")
    
    def update_market_price(self):
        """
        Обновляет рыночную цену с учетом тренда и волатильности
        """
        # Случайное изменение тренда
        if random.random() < 0.1:
            self.price_trend += random.uniform(-0.2, 0.2)
            self.price_trend = max(min(self.price_trend, 1.0), -1.0)
        
        # Изменение волатильности
        if random.random() < 0.05:
            self.volatility = random.uniform(0.05, 0.3)
        
        # Расчет изменения цены
        change = (self.price_trend + random.uniform(-1, 1) * self.volatility)
        self.current_price += change
        
        return self.current_price
    
    def generate_orders_batch(self):
        """
        Генерирует пачку ордеров с реалистичной структурой книги ордеров
        """
        self.update_market_price()
        current_time = int(time.time() * 1000000)
        exchange_ts = current_time
        
        # Количество ордеров в этой пачке
        num_orders = random.randint(3, 12)
        orders = []
        
        # Генерируем уровни цен для bid и ask
        bid_levels = []
        ask_levels = []
        
        for i in range(num_orders // 2 + 1):
            # Bid уровни (покупка)
            bid_price = self.current_price - (i + 1) * random.uniform(0.05, 0.2)
            bid_qty = random.uniform(0.005, 0.03) * (1 + random.random())
            bid_levels.append((bid_price, bid_qty))
            
            # Ask уровни (продажа)
            ask_price = self.current_price + (i + 1) * random.uniform(0.05, 0.2)
            ask_qty = random.uniform(0.005, 0.03) * (1 + random.random())
            ask_levels.append((ask_price, ask_qty))
        
        # Перемешиваем и добавляем ордера
        all_orders = []
        
        # Добавляем bid ордера
        for price, qty in bid_levels:
            if random.random() > 0.3:  # 70% вероятность добавления
                all_orders.append(('bid', price, qty))
        
        # Добавляем ask ордера
        for price, qty in ask_levels:
            if random.random() > 0.3:
                all_orders.append(('ask', price, qty))
        
        # Перемешиваем ордера
        random.shuffle(all_orders)
        
        # Создаем финальные записи
        for i, (side, price, qty) in enumerate(all_orders[:num_orders]):
            receive_delay = random.randint(1, 10)
            
            orders.append({
                'receive_ts': exchange_ts + receive_delay + i,
                'exchange_ts': exchange_ts,
                'price': f"{price:.8f}",
                'quantity': f"{qty:.8f}",
                'side': side
            })
        
        return orders
    
    def write_batch(self, orders):
        """
        Записывает пачку ордеров в файл
        """
        with open(self.filename, 'a', newline='', encoding='utf-8') as csvfile:
            fieldnames = ['receive_ts', 'exchange_ts', 'price', 'quantity', 'side']
            writer = csv.DictWriter(csvfile, fieldnames=fieldnames, delimiter=';')
            
            for order in orders:
                writer.writerow(order)
                self.row_count += 1
    
    def display_stats(self, orders):
        """
        Отображает статистику генерации
        """
        if not self.show_live_preview:
            return
        
        elapsed = time.time() - self.start_time
        rate = self.row_count / elapsed if elapsed > 0 else 0
        
        # Подсчитываем bid/ask
        bids = sum(1 for o in orders if o['side'] == 'bid')
        asks = len(orders) - bids
        
        # Последние цены
        last_bid = next((o['price'] for o in reversed(orders) if o['side'] == 'bid'), 'N/A')
        last_ask = next((o['price'] for o in reversed(orders) if o['side'] == 'ask'), 'N/A')
        
        # Очищаем предыдущую строку и выводим новую
        print(f"\r[{datetime.now().strftime('%H:%M:%S')}] "
              f"Строк: {self.row_count} | "
              f"Скорость: {rate:.1f} строк/с | "
              f"Bid/Ask: {bids}/{asks} | "
              f"Посл. цена: B:{last_bid} A:{last_ask} | "
              f"Рынок: {self.current_price:.2f}", end='', flush=True)
    
    def run(self):
        """
        Запускает непрерывную генерацию
        """
        print("\n" + "="*80)
        print("НЕПРЕРЫВНАЯ ГЕНЕРАЦИЯ ДАННЫХ")
        print("="*80)
        print(f"Файл: {self.filename}")
        print("Нажмите ENTER для остановки и просмотра статистики")
        print("-"*80)
        
        # Поток для отслеживания нажатия Enter
        stop_event = threading.Event()
        
        def wait_for_enter():
            input()
            stop_event.set()
        
        input_thread = threading.Thread(target=wait_for_enter)
        input_thread.daemon = True
        input_thread.start()
        
        try:
            while not stop_event.is_set():
                # Генерируем пачку ордеров
                orders = self.generate_orders_batch()
                
                # Записываем
                self.write_batch(orders)
                
                # Показываем статистику
                self.display_stats(orders)
                
                # Пауза между генерациями
                time.sleep(random.uniform(0.3, 1.5))
                
        except KeyboardInterrupt:
            pass
        
        # Финальная статистика
        self.show_final_stats()
    
    def show_final_stats(self):
        """
        Показывает финальную статистику
        """
        elapsed = time.time() - self.start_time
        rate = self.row_count / elapsed if elapsed > 0 else 0
        file_size = os.path.getsize(self.filename) / 1024  # в KB
        
        print("\n" + "="*80)
        print("СТАТИСТИКА ГЕНЕРАЦИИ")
        print("="*80)
        print(f"Всего строк: {self.row_count}")
        print(f"Время работы: {elapsed:.1f} секунд")
        print(f"Средняя скорость: {rate:.1f} строк/сек")
        print(f"Размер файла: {file_size:.2f} KB")
        print(f"Финальная цена: {self.current_price:.2f}")
        print(f"Файл сохранен: {self.filename}")
        print("="*80)

def main():
    # Можно настроить параметры
    generator = AdvancedOrderGenerator(
        filename='market_data_orders.csv',
        show_live_preview=True
    )
    
    generator.run()

if __name__ == "__main__":
    main()