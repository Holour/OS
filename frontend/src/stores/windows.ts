import { defineStore } from 'pinia';

export interface WindowState {
  id: string;
  title: string;
  component: string;
  props?: Record<string, any>;
  isVisible: boolean;
  isActive: boolean;
  isFocused: boolean;
  position: { x: number; y: number };
  size: { width: number; height: number };
  zIndex: number;
}

export const useWindowsStore = defineStore('windows', {
  state: () => ({
    windows: [] as WindowState[],
    nextZIndex: 100,
  }),
  actions: {
    openWindow(id: string, title: string, component: string, props = {}) {
        const existingWindow = this.windows.find(w => w.id === id);
        if (existingWindow) {
            this.focusWindow(id);
            return;
        }

        const newWindow: WindowState = {
            id,
            title,
            component,
            props,
            isVisible: true,
            isActive: true,
            isFocused: true,
            position: { x: 100, y: 100 },
            size: { width: 640, height: 480 },
            zIndex: this.nextZIndex++,
        };

        this.windows.forEach(w => {
            w.isActive = false;
            w.isFocused = false;
        });
        this.windows.push(newWindow);
    },

    closeWindow(id: string) {
        this.windows = this.windows.filter(w => w.id !== id);
    },

    focusWindow(id: string) {
        this.windows.forEach(w => {
            if (w.id === id) {
                w.isActive = true;
                w.isFocused = true;
                w.zIndex = this.nextZIndex++;
            } else {
                w.isActive = false;
                w.isFocused = false;
            }
        });
    },

    updateWindowPosition(id: string, position: { x: number; y: number }) {
        const win = this.windows.find(w => w.id === id);
        if (win) {
            win.position = position;
        }
    },
  },
});
