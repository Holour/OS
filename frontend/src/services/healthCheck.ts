export interface HealthCheckResult {
  status: 'ok' | 'error';
  timestamp: number;
  services: {
    api: boolean;
    database: boolean;
    filesystem: boolean;
  };
  version?: string;
  uptime?: number;
}

export class HealthCheckService {
  private static instance: HealthCheckService;
  private baseUrl: string;

  constructor(baseUrl: string = 'http://localhost:8080') {
    this.baseUrl = baseUrl;
  }

  static getInstance(baseUrl?: string): HealthCheckService {
    if (!HealthCheckService.instance) {
      HealthCheckService.instance = new HealthCheckService(baseUrl);
    }
    return HealthCheckService.instance;
  }

  async checkHealth(timeout: number = 5000): Promise<HealthCheckResult> {
    const controller = new AbortController();
    const timeoutId = setTimeout(() => controller.abort(), timeout);

    try {
      // 尝试调用进程API作为健康检查
      const response = await fetch(`${this.baseUrl}/api/v1/processes`, {
        method: 'GET',
        signal: controller.signal,
        headers: {
          'Content-Type': 'application/json'
        }
      });

      clearTimeout(timeoutId);

      if (response.ok) {
        const data = await response.json();

        return {
          status: 'ok',
          timestamp: Date.now(),
          services: {
            api: true,
            database: true,
            filesystem: true
          },
          version: '2.1.0',
          uptime: Date.now() // 简化版，实际应该从后端获取
        };
      } else {
        throw new Error(`HTTP ${response.status}: ${response.statusText}`);
      }
    } catch (error: any) {
      clearTimeout(timeoutId);

      let errorMessage = 'Unknown error';

      if (error.name === 'AbortError') {
        errorMessage = 'Connection timeout';
      } else if (error.code === 'ECONNREFUSED' || error.message.includes('fetch')) {
        errorMessage = 'Connection refused - backend service not running';
      } else {
        errorMessage = error.message;
      }

      return {
        status: 'error',
        timestamp: Date.now(),
        services: {
          api: false,
          database: false,
          filesystem: false
        }
      };
    }
  }

  async isBackendAvailable(timeout: number = 5000): Promise<boolean> {
    try {
      const result = await this.checkHealth(timeout);
      return result.status === 'ok';
    } catch {
      return false;
    }
  }

  setBaseUrl(url: string): void {
    this.baseUrl = url;
  }

  getBaseUrl(): string {
    return this.baseUrl;
  }
}
