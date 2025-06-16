import { ref, reactive } from 'vue';

interface MessageDialogOptions {
  type: 'info' | 'success' | 'warning' | 'error' | 'question';
  title: string;
  message: string;
  details?: string;
}

interface InputDialogOptions {
  title: string;
  message?: string;
  inputLabel?: string;
  placeholder?: string;
  defaultValue?: string;
  inputType?: string;
  hint?: string;
  multipleInputs?: boolean;
  inputsConfig?: Array<{
    label?: string;
    value: string;
    type?: string;
    placeholder?: string;
    hint?: string;
    required?: boolean;
  }>;
  validator?: (value: any) => boolean;
}

interface FileProperties {
  name: string;
  type: 'file' | 'directory';
  size?: number;
  simulated_size?: number;
  path: string;
  permissions?: string;
  last_modified?: string;
  created?: string;
  [key: string]: any;
}

// 全局对话框状态
const messageDialog = reactive({
  visible: false,
  type: 'info' as MessageDialogOptions['type'],
  title: '',
  message: '',
  details: '',
  resolve: null as ((value: boolean) => void) | null
});

const inputDialog = reactive({
  visible: false,
  title: '',
  message: '',
  inputLabel: '',
  placeholder: '',
  defaultValue: '',
  inputType: 'text',
  hint: '',
  multipleInputs: false,
  inputsConfig: [] as InputDialogOptions['inputsConfig'],
  validator: null as InputDialogOptions['validator'],
  resolve: null as ((value: any) => void) | null,
  reject: null as (() => void) | null
});

const propertiesDialog = reactive({
  visible: false,
  file: null as FileProperties | null
});

export function useDialogs() {
  // 显示消息对话框
  const showMessage = (options: MessageDialogOptions): Promise<boolean> => {
    return new Promise((resolve) => {
      messageDialog.visible = true;
      messageDialog.type = options.type;
      messageDialog.title = options.title;
      messageDialog.message = options.message;
      messageDialog.details = options.details || '';
      messageDialog.resolve = resolve;
    });
  };

  // 快捷方法
  const alert = (message: string, title = '提示') => {
    return showMessage({ type: 'info', title, message });
  };

  const success = (message: string, title = '成功') => {
    return showMessage({ type: 'success', title, message });
  };

  const warning = (message: string, title = '警告') => {
    return showMessage({ type: 'warning', title, message });
  };

  const error = (message: string, title = '错误') => {
    return showMessage({ type: 'error', title, message });
  };

  const confirm = (message: string, title = '确认') => {
    return showMessage({ type: 'question', title, message });
  };

  // 显示输入对话框
  const prompt = (options: InputDialogOptions): Promise<any> => {
    return new Promise((resolve, reject) => {
      inputDialog.visible = true;
      inputDialog.title = options.title;
      inputDialog.message = options.message || '';
      inputDialog.inputLabel = options.inputLabel || '';
      inputDialog.placeholder = options.placeholder || '';
      inputDialog.defaultValue = options.defaultValue || '';
      inputDialog.inputType = options.inputType || 'text';
      inputDialog.hint = options.hint || '';
      inputDialog.multipleInputs = options.multipleInputs || false;
      inputDialog.inputsConfig = options.inputsConfig || [];
      inputDialog.validator = options.validator || null;
      inputDialog.resolve = resolve;
      inputDialog.reject = reject;
    });
  };

  // 显示属性对话框
  const showProperties = (file: FileProperties) => {
    propertiesDialog.visible = true;
    propertiesDialog.file = file;
  };

  // 关闭对话框的处理函数
  const closeMessageDialog = () => {
    if (messageDialog.resolve) {
      messageDialog.resolve(false);
    }
    messageDialog.visible = false;
    messageDialog.resolve = null;
  };

  const confirmMessageDialog = () => {
    if (messageDialog.resolve) {
      messageDialog.resolve(true);
    }
    messageDialog.visible = false;
    messageDialog.resolve = null;
  };

  const closeInputDialog = () => {
    if (inputDialog.reject) {
      inputDialog.reject();
    }
    inputDialog.visible = false;
    inputDialog.resolve = null;
    inputDialog.reject = null;
  };

  const confirmInputDialog = (value: any) => {
    if (inputDialog.resolve) {
      inputDialog.resolve(value);
    }
    inputDialog.visible = false;
    inputDialog.resolve = null;
    inputDialog.reject = null;
  };

  const closePropertiesDialog = () => {
    propertiesDialog.visible = false;
    propertiesDialog.file = null;
  };

  return {
    // 对话框状态
    messageDialog,
    inputDialog,
    propertiesDialog,

    // 显示方法
    showMessage,
    alert,
    success,
    warning,
    error,
    confirm,
    prompt,
    showProperties,

    // 关闭方法
    closeMessageDialog,
    confirmMessageDialog,
    closeInputDialog,
    confirmInputDialog,
    closePropertiesDialog
  };
}
